//
//  ViewController.swift
//  maxprocmon
//
//  Created by Maxwell on 17/07/22.
//

import Cocoa
import SecurityFoundation
import ServiceManagement

class ViewController: NSViewController {

    @IBOutlet weak var statusLabel: NSTextField!
    
    var xpc: NSXPCConnection?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let xpc = NSXPCConnection(machServiceName: "town.max.maxprocmon-xpc", options: .privileged)
        xpc.exportedInterface = NSXPCInterface(with: maxprocmon_Client.self)
        xpc.exportedObject = XPCClient()
        xpc.remoteObjectInterface = NSXPCInterface(with: maxprocmon_Server.self)
        xpc.interruptionHandler = {
            NSLog("xpc service interrupted")
        }
        xpc.invalidationHandler = {
            NSLog("xpc service invalidated")
        }
        self.xpc = xpc
        xpc.resume()
        (xpc.remoteObjectProxy as? maxprocmon_Server)?.status({ statusString in
            self.statusLabel.stringValue = statusString ?? "xpc not responding"
        })
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }

    @IBAction func runInstall(_ sender: Any) {
        var auth: AuthorizationRef?
        let status: OSStatus = AuthorizationCreate(nil, nil, [], &auth)
        if status != errAuthorizationSuccess {
            NSLog("AuthorizationCreate failed")
            
        } else {
            var error: Unmanaged<CFError>?
            let blessStatus = SMJobBless(kSMDomainSystemLaunchd, "town.max.maxprocmond" as CFString, auth, &error)
            
            if blessStatus {
                
            } else {
                if let error = error?.takeUnretainedValue() {
                    NSLog("SMJobBless failed - \(error)")
                } else {
                    NSLog("SMJobBless failed")
                }
            }
        }
        
        (xpc?.remoteObjectProxy as? maxprocmon_Server)?.install({ ok in
            self.statusLabel.stringValue = "Installed: \(ok)"
        })
    }
    
    @IBAction func runUninstall(_ sender: Any) {
        (xpc?.remoteObjectProxy as? maxprocmon_Server)?.uninstall({ ok in
            self.statusLabel.stringValue = "Uninstalled: \(ok)"
        })
    }
}
