//
//  maxprocmon_xpcProtocol.h
//  maxprocmon-xpc
//
//  Created by Maxwell on 17/07/22.
//

#import <Foundation/Foundation.h>

// The protocol that this service will vend as its API. This header file will also need to be visible to the process hosting the service.
@protocol maxprocmon_Server

- (void)status:(void (^)(NSString *))reply;
- (void)install:(void (^)(bool))reply;
- (void)uninstall:(void (^)(bool))reply;

@end

@protocol maxprocmon_Client

- (void)statusChanged:(NSString *)aString;
    
@end

/*
 To use the service from an application or other process, use NSXPCConnection to establish a connection to the service by doing something like this:

     _connectionToService = [[NSXPCConnection alloc] initWithServiceName:@"town.max.maxprocmon-xpc"];
     _connectionToService.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(maxprocmon_xpcProtocol)];
     [_connectionToService resume];

Once you have a connection to the service, you can use it like this:

     [[_connectionToService remoteObjectProxy] upperCaseString:@"hello" withReply:^(NSString *aString) {
         // We have received a response. Update our text field, but do it on the main thread.
         NSLog(@"Result string was: %@", aString);
     }];

 And, when you are finished with the service, clean up the connection like this:

     [_connectionToService invalidate];
*/
