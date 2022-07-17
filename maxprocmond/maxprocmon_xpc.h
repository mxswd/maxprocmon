//
//  maxprocmon_xpc.h
//  maxprocmon-xpc
//
//  Created by Maxwell on 17/07/22.
//

#import <Foundation/Foundation.h>
#import "maxprocmon_xpcProtocol.h"

// This object implements the protocol which we have defined. It provides the actual behavior for the service. It is 'exported' by the service to make it available to the process hosting the service over an NSXPCConnection.
@interface maxprocmon_xpc : NSObject <maxprocmon_Server>
@end
