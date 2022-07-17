//
//  maxprocmon_xpc.m
//  maxprocmon-xpc
//
//  Created by Maxwell on 17/07/22.
//

#import "maxprocmon_xpc.h"

@implementation maxprocmon_xpc

- (void)status:(void (^)(NSString *))reply {
    reply(@"HELLO");
}

- (void)install:(void (^)(bool))reply {
    reply(true);
}

- (void)uninstall:(void (^)(bool))reply {
    reply(false);
}


@end
