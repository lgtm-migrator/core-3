/*
  Copyright 2022 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <generic_agent.h>

#include <dbm_api.h>
#include <lastseen.h>
#include <dir.h>
#include <scope.h>
#include <files_copy.h>
#include <files_interfaces.h>
#include <hash.h>
#include <keyring.h>
#include <eval_context.h>
#include <crypto.h>
#include <known_dirs.h>
#include <man.h>
#include <signals.h>
#include <string_lib.h>
#include <file_lib.h>           /* FILE_SEPARATOR */
#include <cleanup.h>

#include <cf-key-functions.h>

bool SHOWHOSTS = false;                                         /* GLOBAL_A */
bool NO_TRUNCATE = false;                                       /* GLOBAL_A */
bool FORCEREMOVAL = false;                                      /* GLOBAL_A */
bool REMOVEKEYS = false;                                        /* GLOBAL_A */
bool LICENSE_INSTALL = false;                                   /* GLOBAL_A */
char LICENSE_SOURCE[MAX_FILENAME] = "";                         /* GLOBAL_A */
const char *remove_keys_host = NULL;                            /* GLOBAL_A */
static char *print_digest_arg = NULL;                           /* GLOBAL_A */
static char *trust_key_arg = NULL;                              /* GLOBAL_A */
static char *KEY_PATH = NULL;                                   /* GLOBAL_A */
static int KEY_SIZE = 2048;                                     /* GLOBAL_A */
bool LOOKUP_HOSTS = true;                                       /* GLOBAL_A */

static GenericAgentConfig *CheckOpts(int argc, char **argv);

/*******************************************************************/
/* Command line options                                            */
/*******************************************************************/

static const char *const CF_KEY_SHORT_DESCRIPTION =
    "make private/public key-pairs for CFEngine authentication";

static const char *const CF_KEY_MANPAGE_LONG_DESCRIPTION =
    "The CFEngine key generator makes key pairs for remote authentication.\n";

#define TIMESTAMP_VAL 1234 // Anything outside ASCII range.
static const struct option OPTIONS[] =
{
    {"help", no_argument, 0, 'h'},
    {"inform", no_argument, 0, 'I'},
    {"debug", no_argument, 0, 'd'},
    {"verbose", no_argument, 0, 'v'},
    {"version", no_argument, 0, 'V'},
    {"log-level", required_argument, 0, 'g'},
    {"output-file", required_argument, 0, 'f'},
    {"key-type", required_argument, 0, 'T'},
    {"show-hosts", no_argument, 0, 's'},
    {"no-truncate", no_argument, 0, 'N'},
    {"remove-keys", required_argument, 0, 'r'},
    {"force-removal", no_argument, 0, 'x'},
    {"install-license", required_argument, 0, 'l'},
    {"print-digest", optional_argument, 0, 'p'},
    {"trust-key", required_argument, 0, 't'},
    {"color", optional_argument, 0, 'C'},
    {"timestamp", no_argument, 0, TIMESTAMP_VAL},
    {"numeric", no_argument, 0, 'n'},
    {NULL, 0, 0, '\0'}
};

static const char *const HINTS[] =
{
    "Print the help message",
    "Print basic information about key generation",
    "Enable debugging output",
    "Output verbose information about the behaviour of cf-key",
    "Output the version of the software",
    "Specify how detailed logs should be. Possible values: 'error', 'warning', 'notice', 'info', 'verbose', 'debug'",
    "Specify an alternative output file than the default.",
    "Specify a RSA key size in bits, the default value is 2048.",
    "Show lastseen hostnames and IP addresses",
    "Don't truncate -s / --show-hosts output",
    "Remove keys for specified hostname/IP/MD5/SHA (cf-key -r SHA=12345, cf-key -r MD5=12345, cf-key -r host001, cf-key -r 203.0.113.1)",
    "Force removal of keys",
    "Install license file on Enterprise server (CFEngine Enterprise Only)",
    "Print digest of the specified public key",
    "Make cf-serverd/cf-agent trust the specified public key. Argument value is of the form [[USER@]IPADDR:]FILENAME where FILENAME is the local path of the public key for client at IPADDR address.",
    "Enable colorized output. Possible values: 'always', 'auto', 'never'. If option is used, the default value is 'auto'",
    "Log timestamps on each line of log output",
    "Do not lookup host names",
    NULL
};

/*****************************************************************************/

typedef void (*CfKeySigHandler)(int signum);
bool cf_key_interrupted = false;

static void handleShowKeysSignal(int signum)
{
    cf_key_interrupted = true;

    signal(signum, handleShowKeysSignal);
}

static void SetupSignalsForCfKey(CfKeySigHandler sighandler)
{
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGBUS, HandleSignalsForAgent);
    signal(SIGUSR1, HandleSignalsForAgent);
    signal(SIGUSR2, HandleSignalsForAgent);
}

int main(int argc, char *argv[])
{
    SetupSignalsForCfKey(HandleSignalsForAgent);

    GenericAgentConfig *config = CheckOpts(argc, argv);
    EvalContext *ctx = EvalContextNew();
    GenericAgentConfigApply(ctx, config);

    const char *program_invocation_name = argv[0];
    const char *last_dir_sep = strrchr(program_invocation_name, FILE_SEPARATOR);
    const char *program_name = (last_dir_sep != NULL ? last_dir_sep + 1 : program_invocation_name);
    GenericAgentDiscoverContext(ctx, config, program_name);

    if (SHOWHOSTS)
    {
        SetupSignalsForCfKey(handleShowKeysSignal);
        ShowLastSeenHosts(!NO_TRUNCATE);
        GenericAgentFinalize(ctx, config);
        CallCleanupFunctions();
        return EXIT_SUCCESS;
    }

    if (print_digest_arg)
    {
        GenericAgentFinalize(ctx, config);
        CallCleanupFunctions();
        int rc = PrintDigest(print_digest_arg);
        free(print_digest_arg);
        return rc;
    }

    GenericAgentPostLoadInit(ctx);

    if (REMOVEKEYS)
    {
        int status;
        if (FORCEREMOVAL)
        {
            if (!strncmp(remove_keys_host, "SHA=", 3) ||
                !strncmp(remove_keys_host, "MD5=", 3))
            {
                status = ForceKeyRemoval(remove_keys_host);
            }
            else
            {
                status = ForceIpAddressRemoval(remove_keys_host);
            }
        }
        else
        {
            status = RemoveKeys(remove_keys_host, true);
            if (status == 0 || status == 1)
            {
                Log (LOG_LEVEL_VERBOSE,
                     "Forced removal of entry '%s' was successful",
                     remove_keys_host);
                status = EXIT_SUCCESS;
            }
        }
        GenericAgentFinalize(ctx, config);
        CallCleanupFunctions();
        return status;
    }

    if(LICENSE_INSTALL)
    {
        bool success = LicenseInstall(LICENSE_SOURCE);
        GenericAgentFinalize(ctx, config);
        CallCleanupFunctions();
        return success ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (trust_key_arg != NULL)
    {
        char *filename, *ipaddr, *username;
        /* We will modify the argument to --trust-key. */
        char *arg = xstrdup(trust_key_arg);

        ParseKeyArg(arg, &filename, &ipaddr, &username);

        /* Server IP address required to trust key on the client side. */
        if (ipaddr == NULL)
        {
            Log(LOG_LEVEL_NOTICE, "Establishing trust might be incomplete. "
                "For completeness, use --trust-key IPADDR:filename");
        }

        bool ret = TrustKey(filename, ipaddr, username);

        free(arg);
        GenericAgentFinalize(ctx, config);
        CallCleanupFunctions();
        return ret ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    char *public_key_file, *private_key_file;

    if (KEY_PATH)
    {
        xasprintf(&public_key_file, "%s.pub", KEY_PATH);
        xasprintf(&private_key_file, "%s.priv", KEY_PATH);
    }
    else
    {
        public_key_file = PublicKeyFile(GetWorkDir());
        private_key_file = PrivateKeyFile(GetWorkDir());
    }

    bool ret = KeepKeyPromises(public_key_file, private_key_file, KEY_SIZE);

    free(public_key_file);
    free(private_key_file);

    GenericAgentFinalize(ctx, config);

    CallCleanupFunctions();
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*****************************************************************************/
/* Level                                                                     */
/*****************************************************************************/

static void PrintHelp()
{
    Writer *w = FileWriter(stdout);
    WriterWriteHelp(w, "cf-key", OPTIONS, HINTS, NULL, false, false);
    FileWriterDetach(w);
}

static GenericAgentConfig *CheckOpts(int argc, char **argv)
{
    extern char *optarg;
    int c;
    GenericAgentConfig *config = GenericAgentConfigNewDefault(AGENT_TYPE_KEYGEN, GetTTYInteractive());

    while ((c = getopt_long(argc, argv, "dvIf:g:T:VMp::sNr:xt:hl:C::n",
                            OPTIONS, NULL))
           != -1)
    {
        switch (c)
        {
        case 'f':
            KEY_PATH = optarg;
            break;

        case 'T':
            KEY_SIZE = StringToLongExitOnError(optarg);
            break;

        case 'd':
            LogSetGlobalLevel(LOG_LEVEL_DEBUG);
            break;

        case 'V':
            {
                Writer *w = FileWriter(stdout);
                GenericAgentWriteVersion(w);
                FileWriterDetach(w);
            }
            GenericAgentConfigDestroy(config);
            DoCleanupAndExit(EXIT_SUCCESS);

        case 'v':
            LogSetGlobalLevel(LOG_LEVEL_VERBOSE);
            break;

        case 'g':
            LogSetGlobalLevelArgOrExit(optarg);
            break;

        case 'I':
            LogSetGlobalLevel(LOG_LEVEL_INFO);
            break;

        case 'p': /* print digest */
            if (OPTIONAL_ARGUMENT_IS_PRESENT)
            {
                print_digest_arg = xstrdup(optarg);
            }
            else
            {
                print_digest_arg = PublicKeyFile(GetWorkDir());
            }
            break;

        case 's':
            SHOWHOSTS = true;
            break;

        case 'N':
            NO_TRUNCATE = true;
            break;

        case 'x':
            FORCEREMOVAL = true;
            break;

        case 'r':
            REMOVEKEYS = true;
            remove_keys_host = optarg;
            break;

        case 'l':
            LICENSE_INSTALL = true;
            strlcpy(LICENSE_SOURCE, optarg, sizeof(LICENSE_SOURCE));
            break;

        case 't':
            trust_key_arg = optarg;
            break;

        case 'h':
            PrintHelp();
            GenericAgentConfigDestroy(config);
            DoCleanupAndExit(EXIT_SUCCESS);

        case 'M':
            {
                Writer *out = FileWriter(stdout);
                ManPageWrite(out, "cf-key", time(NULL),
                             CF_KEY_SHORT_DESCRIPTION,
                             CF_KEY_MANPAGE_LONG_DESCRIPTION,
                             OPTIONS, HINTS,
                             NULL, false,
                             false);
                FileWriterDetach(out);
                GenericAgentConfigDestroy(config);
                DoCleanupAndExit(EXIT_SUCCESS);
            }

        case 'C':
            if (!GenericAgentConfigParseColor(config,
                (OPTIONAL_ARGUMENT_IS_PRESENT) ? optarg : "auto"))
            {
                GenericAgentConfigDestroy(config);
                DoCleanupAndExit(EXIT_FAILURE);
            }
            break;

        case TIMESTAMP_VAL:
            LoggingEnableTimestamps(true);
            break;

        case 'n':
            LOOKUP_HOSTS = false;
            break;

        default:
            PrintHelp();
            GenericAgentConfigDestroy(config);
            DoCleanupAndExit(EXIT_FAILURE);

        }
    }

    if (NO_TRUNCATE && !SHOWHOSTS)
    {
        PrintHelp();
        Log(LOG_LEVEL_ERR, "--no-truncate / -N option is only for --show-hosts / -s");
        GenericAgentConfigDestroy(config);
        DoCleanupAndExit(EXIT_FAILURE);
    }

    return config;
}
