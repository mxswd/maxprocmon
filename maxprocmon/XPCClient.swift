//
//  XPCClient.swift
//  maxprocmon
//
//  Created by Maxwell on 17/07/22.
//

import Cocoa

class XPCClient: NSObject, maxprocmon_Client {
    func statusChanged(_ aString: String!) {
        print(aString)
    }
}
