#include "logging.hpp"
#include "restarter.hpp"
#include <string>
#include <iostream>
#include <bits/stdc++.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

using namespace std;

bool restart;
pid_t cpid;
pid_t ppid;

void sig_handler(int signo)
{
    if (cpid != 0) kill (cpid, signo);
}

void install_signalhandler(void (*handler) (int))
{

   int sig;
    for (sig = 1; sig < NSIG; sig++)
    {
        if (signal(sig, handler) == SIG_ERR) {
            logging::TRACE(str_pf ("Can't catch %i -> %s", sig, strsignal(sig)));
            //this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void forkAndExec (vector<string> myTokens)
{

    vector<const char *> chars(myTokens.size());
    transform(myTokens.begin(), myTokens.end(), chars.begin(), mem_fun_ref(&string::c_str));

    char* param_list[myTokens.size()];
    //param_list[0] = &(cfg.name[0]);
    for (uint cntr = 0; cntr < chars.size(); cntr++)
        param_list[cntr] = (char*) chars[cntr];
    param_list[myTokens.size()] = 0;

    char* executable = &(myTokens[0][0]);
    //char* executable = &(param.executable[0]);

    logging::DEBUG(str_pf("Executable: %s   Name: %s\n", executable, param_list[0]));

    //execv(executable, param_list);

    pid_t ret = fork();
    if(ret==0)
    {
       //child process
       execv(executable, param_list);
       printf("EXECV Failed from child\n");
    }
    else if(ret>0)
    {
        cpid = ret;
       //parent process
       printf("Parent to wait\n");
    }
    else
    {
       //you will come here only if fork() fails.
       printf("forkFailed\n");
    }

}

void printDate (tm *test)
{
    cout << "Nächster Start" << endl;
    cout << test->tm_mday << ".";
    cout << 1 + test->tm_mon << ".";
    cout << 1900 + test->tm_year<< ".";

    cout << " "<< test->tm_hour << ":";
    cout << test->tm_min << ":";
    cout << test->tm_sec << endl;
}

time_t calcNextStart (int l_hour, int l_min, int l_sec)
{
    time_t now = time(0);
    time_t tmp = now;
    tm *ltm = localtime(&tmp);
    ltm->tm_hour = l_hour;
    ltm->tm_min = l_min;
    ltm->tm_sec = l_sec;

    time_t t1 = mktime(ltm);

    if (t1 > now) return (t1);

    t1 = t1 + 86400;

    return (t1);
}

void shutdown (int grace)
{
    kill (cpid, 15);
    chrono::steady_clock::time_point start = chrono::steady_clock::now();
    while (true)
    {
        int wret = waitpid (-1, 0, WNOHANG);
        if (wret !=0) break;
        chrono::seconds duration =  std::chrono::duration_cast<std::chrono::seconds>(chrono::steady_clock::now() - start );
        if (duration.count() > grace)
        {
             kill (cpid, 9);
             break;
        }
    }
}
int main(int argc, char **argv, char **env)
{

    ppid = getpid();

    logging::setLogLevel ("DEBUG");

    // Vector of string to save tokens
    vector <string> myTokens;
    string l_loglevel;

    ParamTyp param;

    int c;

    while ( (c = getopt(argc, argv, "l:h:m:g:e:p:")) != -1) {
        switch (c) {
        case 'l':
            logging::TRACE (str_pf ("option l with value '%s'", optarg));
            l_loglevel = string(optarg);
            break;
        case 'h':
            logging::TRACE (str_pf ("option h with value '%s'", optarg));
            param.hour = atoi(optarg);
            break;
       case 'm':
            logging::TRACE (str_pf ("option m with value '%s'", optarg));
            param.min = atoi(optarg);
            break;
       case 'g':
            logging::TRACE (str_pf ("option g with value '%s'", optarg));
            param.grace = atoi(optarg);
            break;
        case 'e':
            logging::TRACE (str_pf ("option e with value '%s'", optarg));
            param.executable = string(optarg);
            break;
        case 'p':
            logging::TRACE (str_pf ("option p with value '%s'", optarg));
            param.param = string(optarg);
            break;
        default:
            logging::DEBUG (str_pf ("?? getopt returned character code 0%o ??", c));
        }
    }
    if (optind < argc) {
        logging::DEBUG("non-option ARGV-elements: ");
        while (optind < argc)
        {
            logging::DEBUG(str_pf ("%s ", argv[optind++])); //this_thread::sleep_for(chrono::milliseconds(10));
            printf ("Index: %i\n", optind);
        }
    }

    logging::setLogLevel (l_loglevel);

    stringstream check1(param.param);
    string intermediate;
    // Tokenizing w.r.t. space ' '
    while(getline(check1, intermediate, ' ')) myTokens.push_back(intermediate);

    vector<string>::iterator it;
    it = myTokens.begin();
    it = myTokens.insert ( it , param.executable );


    int l_hour = param.hour;
    int l_min = param.min;
    int l_sec = 0;

    //time_t next = calcNextStart (l_hour, l_min, l_sec);
    //printDate (localtime(&next));

    install_signalhandler(sig_handler);

    while (true)
    {
        restart = false;
        forkAndExec (myTokens);
        if (ppid != getpid()) return(0);
        int wret;
        time_t next = calcNextStart (l_hour, l_min, l_sec);
        printDate (localtime(&next));

        while (true)
        {
            wret = waitpid (-1, 0, WNOHANG);
            if (wret != 0) break;
            if ((time(0) > next) && (wret == 0))
            {
                restart = true;
                shutdown (param.grace);

            }
            this_thread::sleep_for(std::chrono::milliseconds(500));

        }
        if (!restart) return (0);
    }

    return(0);
}

string str_pf(const string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}
