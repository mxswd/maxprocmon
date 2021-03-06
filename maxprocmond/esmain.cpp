// From the https://github.com/gyunaev/macprocmon project

//MIT License
//
//Copyright (c) 2021 George Yunaev.
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <iostream>
#include <vector>
#include <mutex>
#include <unistd.h>

#include "EndpointSecurity.h"
#include "sqlite3.h"

// First is a notify event, second is an auth event or ES_EVENT_TYPE_LAST if there is no auth event
typedef std::tuple<unsigned int, unsigned int> helpdata;

static std::map< std::string, helpdata > supportedEvents = {
        { "access", { ES_EVENT_TYPE_NOTIFY_ACCESS, ES_EVENT_TYPE_LAST } },
        { "chdir", { ES_EVENT_TYPE_NOTIFY_CHDIR, ES_EVENT_TYPE_AUTH_CHDIR } },
        { "chroot", { ES_EVENT_TYPE_NOTIFY_CHROOT, ES_EVENT_TYPE_AUTH_CHROOT } },
        { "clone", { ES_EVENT_TYPE_NOTIFY_CLONE, ES_EVENT_TYPE_AUTH_CLONE } },
        { "close", { ES_EVENT_TYPE_NOTIFY_CLOSE, ES_EVENT_TYPE_LAST } },
        { "create", { ES_EVENT_TYPE_NOTIFY_CREATE, ES_EVENT_TYPE_AUTH_CREATE } },
        { "deleteextattr", { ES_EVENT_TYPE_NOTIFY_DELETEEXTATTR, ES_EVENT_TYPE_AUTH_DELETEEXTATTR } },
        { "dup", { ES_EVENT_TYPE_NOTIFY_DUP, ES_EVENT_TYPE_LAST } },
        { "exchangedata", { ES_EVENT_TYPE_NOTIFY_EXCHANGEDATA, ES_EVENT_TYPE_AUTH_EXCHANGEDATA } },
        { "exec", { ES_EVENT_TYPE_NOTIFY_EXEC, ES_EVENT_TYPE_AUTH_EXEC } },
        { "exit", { ES_EVENT_TYPE_NOTIFY_EXIT, ES_EVENT_TYPE_LAST } },
        { "fcntl", { ES_EVENT_TYPE_NOTIFY_FCNTL, ES_EVENT_TYPE_AUTH_FCNTL } },
        { "file_provider_materialize", { ES_EVENT_TYPE_NOTIFY_FILE_PROVIDER_MATERIALIZE, ES_EVENT_TYPE_AUTH_FILE_PROVIDER_MATERIALIZE } },
        { "file_provider_update", { ES_EVENT_TYPE_NOTIFY_FILE_PROVIDER_UPDATE, ES_EVENT_TYPE_AUTH_FILE_PROVIDER_UPDATE } },
        { "fork", { ES_EVENT_TYPE_NOTIFY_FORK, ES_EVENT_TYPE_LAST } },
        { "fsgetpath", { ES_EVENT_TYPE_NOTIFY_FSGETPATH, ES_EVENT_TYPE_AUTH_FSGETPATH } },
        { "getattrlist", { ES_EVENT_TYPE_NOTIFY_GETATTRLIST, ES_EVENT_TYPE_AUTH_GETATTRLIST } },
        { "getextattr", { ES_EVENT_TYPE_NOTIFY_GETEXTATTR, ES_EVENT_TYPE_AUTH_GETEXTATTR } },
        { "get_task", { ES_EVENT_TYPE_NOTIFY_GET_TASK, ES_EVENT_TYPE_AUTH_GET_TASK } },
        { "iokit_open", { ES_EVENT_TYPE_NOTIFY_IOKIT_OPEN, ES_EVENT_TYPE_AUTH_IOKIT_OPEN } },
        { "kextload", { ES_EVENT_TYPE_NOTIFY_KEXTLOAD, ES_EVENT_TYPE_AUTH_KEXTLOAD } },
        { "kextunload", { ES_EVENT_TYPE_NOTIFY_KEXTUNLOAD, ES_EVENT_TYPE_LAST } },
        { "link", { ES_EVENT_TYPE_NOTIFY_LINK, ES_EVENT_TYPE_AUTH_LINK } },
        { "listextattr", { ES_EVENT_TYPE_NOTIFY_LISTEXTATTR, ES_EVENT_TYPE_AUTH_LISTEXTATTR } },
        { "lookup", { ES_EVENT_TYPE_NOTIFY_LOOKUP, ES_EVENT_TYPE_LAST } },
        { "mmap", { ES_EVENT_TYPE_NOTIFY_MMAP, ES_EVENT_TYPE_AUTH_MMAP } },
        { "mount", { ES_EVENT_TYPE_NOTIFY_MOUNT, ES_EVENT_TYPE_AUTH_MOUNT } },
        { "mprotect", { ES_EVENT_TYPE_NOTIFY_MPROTECT, ES_EVENT_TYPE_AUTH_MPROTECT } },
        { "open", { ES_EVENT_TYPE_NOTIFY_OPEN, ES_EVENT_TYPE_AUTH_OPEN } },
        { "proc_check", { ES_EVENT_TYPE_NOTIFY_PROC_CHECK, ES_EVENT_TYPE_AUTH_PROC_CHECK } },
        { "pty_close", { ES_EVENT_TYPE_NOTIFY_PTY_CLOSE, ES_EVENT_TYPE_LAST } },
        { "pty_grant", { ES_EVENT_TYPE_NOTIFY_PTY_GRANT, ES_EVENT_TYPE_LAST } },
        { "readdir", { ES_EVENT_TYPE_NOTIFY_READDIR, ES_EVENT_TYPE_AUTH_READDIR } },
        { "readlink", { ES_EVENT_TYPE_NOTIFY_READLINK, ES_EVENT_TYPE_AUTH_READLINK } },
        { "rename", { ES_EVENT_TYPE_NOTIFY_RENAME, ES_EVENT_TYPE_AUTH_RENAME } },
        { "setacl", { ES_EVENT_TYPE_NOTIFY_SETACL, ES_EVENT_TYPE_AUTH_SETACL } },
        { "setattrlist", { ES_EVENT_TYPE_NOTIFY_SETATTRLIST, ES_EVENT_TYPE_AUTH_SETATTRLIST } },
        { "setextattr", { ES_EVENT_TYPE_NOTIFY_SETEXTATTR, ES_EVENT_TYPE_AUTH_SETEXTATTR } },
        { "setflags", { ES_EVENT_TYPE_NOTIFY_SETFLAGS, ES_EVENT_TYPE_AUTH_SETFLAGS } },
        { "setmode", { ES_EVENT_TYPE_NOTIFY_SETMODE, ES_EVENT_TYPE_AUTH_SETMODE } },
        { "setowner", { ES_EVENT_TYPE_NOTIFY_SETOWNER, ES_EVENT_TYPE_AUTH_SETOWNER } },
        { "settime", { ES_EVENT_TYPE_NOTIFY_SETTIME, ES_EVENT_TYPE_AUTH_SETTIME } },
        { "signal", { ES_EVENT_TYPE_NOTIFY_SIGNAL, ES_EVENT_TYPE_AUTH_SIGNAL } },
        { "stat", { ES_EVENT_TYPE_NOTIFY_STAT, ES_EVENT_TYPE_LAST } },
        { "truncate", { ES_EVENT_TYPE_NOTIFY_TRUNCATE, ES_EVENT_TYPE_AUTH_TRUNCATE } },
        { "uipc_bind", { ES_EVENT_TYPE_NOTIFY_UIPC_BIND, ES_EVENT_TYPE_AUTH_UIPC_BIND } },
        { "uipc_connect", { ES_EVENT_TYPE_NOTIFY_UIPC_CONNECT, ES_EVENT_TYPE_AUTH_UIPC_CONNECT } },
        { "unlink", { ES_EVENT_TYPE_NOTIFY_UNLINK, ES_EVENT_TYPE_AUTH_UNLINK } },
        { "unmount", { ES_EVENT_TYPE_NOTIFY_UNMOUNT, ES_EVENT_TYPE_LAST } },
        { "utimes", { ES_EVENT_TYPE_NOTIFY_UTIMES, ES_EVENT_TYPE_AUTH_UTIMES } },
        { "write", { ES_EVENT_TYPE_NOTIFY_WRITE, ES_EVENT_TYPE_LAST } }
};

static int event_callback(sqlite3 *db, sqlite3_stmt *pStmt, const EndpointSecurity::Event& event )
{
//    if (event.process_is_es_client) {
//        return 0;
//    }
    static std::mutex m;
    std::lock_guard<std::mutex> lockGuard(m);
    
    sqlite3_bind_text(pStmt, 1, event.event.data(), event.event.length(), NULL);
    sqlite3_bind_double(pStmt, 2, event.time_s);
    sqlite3_bind_double(pStmt, 3, event.time_ns);
    sqlite3_bind_text(pStmt, 4, event.process_executable.data(), event.process_executable.length(), NULL);
    if (event.filename.length() == 0) {
//        if (event.process_team_id.length() > 0) {
//            sqlite3_bind_text(pStmt, 5, event.process_team_id.data(), event.process_team_id.length(), NULL);
//        }
        std::string msg = "<missing>";
        sqlite3_bind_text(pStmt, 5, msg.data(), msg.length(), NULL);
    } else {
        sqlite3_bind_text(pStmt, 5, event.filename.data(), event.filename.length(), NULL);
    }
    
    int rc = sqlite3_step(pStmt);
    sqlite3_reset(pStmt);
    
    std::cout << "event : " << event.event << "\n" << "  time: " << event.timestamp << "\n";

    for ( auto k : event.parameters )
        std::cout << "  " <<  k.first << " : " << k.second << "\n";
    
    std::cout << " process:\n"
        << "        PID : " << event.process_pid << "\n"
        << "       EUID : " << event.process_euid << "\n"
        << "       EGID : " << event.process_egid << "\n"
        << "       PPID : " << event.process_ppid << "\n";

    if ( event.process_ruid != event.process_euid )
        std::cout << "       RUID : " << event.process_ruid << "\n";

    if ( event.process_rgid != event.process_egid )
        std::cout << "       RGID : " << event.process_rgid << "\n";

    if ( event.process_oppid != event.process_ppid )
        std::cout << "      OPPID : " << event.process_oppid << "\n";
        
    std::cout
        << "        GID : " << event.process_gid << "\n"
        << "        SID : " << event.process_sid << "\n"
        << "   threadid : " << event.process_sid << "\n"
        << "       path : " << event.process_executable << "\n"
        << "    csflags : " << event.process_csflags_desc << "\n"
        << "    sign_id : " << event.process_signing_id << "\n"
        << "    started : " << event.process_start_time << "\n"
        << "      extra : " << (event.process_is_platform_binary ? "(platform_binary) " : "")
                            << (event.process_is_es_client ? "(es_client) " : "") << "\n";
                            
    if ( !event.process_team_id.empty() )
        std::cout << "    team_id : " << event.process_team_id << "\n";
    
    std::cout << "\n";
    return 0;
}

// This function tries to create as many clients as possible
static void test_max_clients()
{
    unsigned int client = 0;
    
    try
    {
        for ( ;; )
        {
            EndpointSecurity * e = new EndpointSecurity();
            e->create( [=](const EndpointSecurity::Event& event){ return 0; });
            client++;
        }
    }
    catch ( EndpointSecurityException ex )
    {
        if ( ex.errorCode != ES_NEW_CLIENT_RESULT_ERR_TOO_MANY_CLIENTS )
            std::cerr << "Exception caught in code: " << ex.errorMsg << ", code " << ex.errorCode << "\n";
        else
            std::cerr << "You have successfully created " << client << " clients\n";
        
        exit(1);
    }
    
}

static void help( const char * exe )
{
    std::cout << "Usage: " << exe << "[options]\n"
        "  -e <event> an event to listen for. Can be used multiple times. -e all lists to all events\n"
        "               for example, -e chdir -e +open -e close\n"
        "              + in front of event means it will be handled as auth event\n"
        " -p <path>   only monitor processes started from this path (including subpaths)\n"
        "  --test-max-clients   tests you how many clients you can create\n";
    
    std::cout << "\nEvents you can listen to:\n";

    for ( auto e : supportedEvents )
    {
        if ( std::get<1>(e.second) != ES_EVENT_TYPE_LAST )
            std::cout << "    [+]" << e.first << "\n";
        else
            std::cout << "      " << e.first << "\n";
    }
}


void es_main ( int argc, char ** argv )
{
    std::string monitoredPath;
    std::vector< es_event_type_t > subscriptions;
    unsigned int totalClients = 1;
    bool verbose = false;
    
    if ( argc == 1 )
    {
        help( argv[0] );
        exit( 0 );
    }
    
    // Parse the command-line
    for ( int ca = 1; ca < argc; ca++ )
    {
        // avoids strcmp below
        std::string arg = argv[ca];
        
        if ( arg == "-e" || arg == "--event" )
        {
            if ( ++ca >= argc )
            {
                std::cerr << "-e requires an argument\n";
                exit(1);
            }
            
            // Parse the event list, if any
            arg = argv[ca];
            
            // [rant] it is time to create std::string::split!
            std::string::size_type offset = 0;
            
            while ( offset < arg.length() )
            {
                std::string::size_type newoffset = arg.find( ',', offset );
                
                if ( newoffset == std::string::npos )
                    newoffset = arg.length();

                std::string ev = arg.substr( offset, newoffset - offset );

                if ( ev == "all" )
                {
                    // add all events
                    for ( auto e : supportedEvents )
                    {
                        subscriptions.push_back( (es_event_type_t) std::get<0>( e.second ) );
                    }
                    
                    if ( verbose )
                        std::cout << "Added all notify events for monitoring\n";
                }
                else if ( ev == "+all" )
                {
                    // add all events
                    for ( auto e : supportedEvents )
                    {
                        subscriptions.push_back( (es_event_type_t) std::get<1>( e.second ) );
                    }
                    
                    if ( verbose )
                        std::cout << "Added all auth events for monitoring\n";
                }
                else
                {
                    bool authevent = false;
                    bool remove = false;

                    // could be "-+open" to remove auth open event
                    while ( ev.length() > 0 && (ev[0] == '+' || ev[0] == '-') )
                    {
                        if ( ev[0] == '+' )
                            authevent = true;
                        else
                            remove = true;
                        
                        ev.erase( ev.begin() );
                    }
            
                    auto it = supportedEvents.find( ev );
            
                    if ( it == supportedEvents.end() )
                    {
                        std::cerr << "Unknown event: " << ev << "\n";
                        exit( 1 );
                    }
            
                    if ( remove )
                    {
                        auto rit = std::find( subscriptions.begin(),
                                              subscriptions.end(),
                                              authevent ? (es_event_type_t) std::get<1>( it->second ) : (es_event_type_t) std::get<0>( it->second ) );
                        
                        if ( rit == subscriptions.end() )
                        {
                            std::cerr << "You're trying to remove an event " << ev << " which wasn't added. use -e all,-open\n";
                            exit( 1 );
                        }
                     
                        subscriptions.erase( rit );
                        
                        if ( verbose )
                            std::cout << "Removed from monitoring " << (authevent ? "auth" : "") << " event " << ev << "\n";
                    }
                    else
                    {
                        // Add an event for monitoring
                        subscriptions.push_back( authevent ? (es_event_type_t) std::get<1>( it->second ) : (es_event_type_t) std::get<0>( it->second ) );
                        
                        if ( verbose )
                            std::cout << "Added monitoring " << (authevent ? "auth" : "") << " event " << ev << "\n";
                    }
                }
                
                offset = newoffset + 1;
            }
        }
        else if ( arg == "-p" )
        {
            if ( ++ca >= argc )
            {
                std::cerr << "-p requires an argument\n";
                exit(1);
            }

            monitoredPath = argv[ca];
        }
        else if ( arg == "-c" )
        {
            // internal option to test various things
            if ( ++ca >= argc )
            {
                std::cerr << "-c requires an argument\n";
                exit(1);
            }

            totalClients = std::stoi( argv[ca] );
        }
        else if ( arg == "--help" )
        {
            help( argv[0] );
            exit( 1 );
        }
        else if ( arg == "--test-max-clients" )
        {
            test_max_clients();
            exit( 1 );
        }
        else if ( arg == "-v" )
        {
            verbose = true;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n";
            exit( 1 );
        }
    }
    
    try
    {
        if ( verbose )
            std::cout << "Starting the interceptor using " << totalClients << " EPS clients\n";
        
        sqlite3 *db;
        char *err_msg = 0;
        
        int rc = sqlite3_open("/Library/Application Support/maxprocmon/database.db", &db);
        
        if (rc != SQLITE_OK) {
                
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            
            return;
        }
        rc = sqlite3_exec(db, "PRAGMA journal_mode = WAL;",0,0,0);

        if (rc != SQLITE_OK) {
                
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            
            return;
        }
        
        rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Logs(EventType TEXT, Timestamp DATETIME, TimeNS REAL, Executable TEXT, Filename TEXT);", 0, 0, &err_msg);
        
        if (rc != SQLITE_OK ) {
            fprintf(stderr, "Failed to create table\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            
        } else {
            fprintf(stdout, "Table Friends created successfully\n");
        }
        
        sqlite3_stmt *pStmt;
        rc = sqlite3_prepare_v2(db, "INSERT INTO Logs(EventType, Timestamp, TimeNS, Executable, Filename) VALUES(?, ?, ?, ?, ?)", -1, &pStmt, 0);
        
        if (rc != SQLITE_OK) {
            
            fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(db));
            
            return;
        }
        
        
        for ( unsigned int i = 0; i < totalClients; i++ )
        {
            EndpointSecurity * epsec = new EndpointSecurity();
                
            if ( !monitoredPath.empty() )
                epsec->monitorOnlyProcessPath( monitoredPath );
                
            epsec->create( [=](const EndpointSecurity::Event& event){ return event_callback( db, pStmt, event ); });
            epsec->subscribe( subscriptions );
        }
            
        if ( verbose )
            std::cout << "Intercepting started\n";

        pause();
    }
    catch ( EndpointSecurityException ex )
    {
        std::cerr << "Exception caught in code: " << ex.errorMsg << ", code " << ex.errorCode << "\n";
        exit(1);
    }
}
