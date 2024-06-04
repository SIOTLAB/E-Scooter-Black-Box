//
//  DeviceDiscoveryVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 3/22/24.
//

import UIKit
import STCore
import STBlueSDK

class DeviceDiscoveryVC: UIViewController, BlueDelegate, UITableViewDelegate, UITableViewDataSource {
        
    var parentVC: HomeVC?
    var devices: [Node] = []
    @IBOutlet weak var tableView: UITableView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        tableView.delegate = self
        tableView.dataSource = self
        tableView.register(UINib(nibName: "TableViewCell", bundle: nil), forCellReuseIdentifier: "reusableDeviceCell")
        tableView.rowHeight = 80
        
        BlueManager.shared.addDelegate(self)
        BlueManager.shared.discoveryStart()
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        self.view.frame = CGRect(x: 0, y: UIScreen.main.bounds.height / 5 * 2, width: self.view.bounds.width, height: UIScreen.main.bounds.height / 5 * 3)
        self.view.layer.cornerRadius = 20
        self.view.layer.masksToBounds = true
        
    }
    
    func justConnected() {
        BlueManager.shared.removeDelegate(self)
        BlueManager.shared.discoveryStop()
        // further visual updates
    }
    
    // MARK: UITableView Methods
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return devices.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "reusableDeviceCell") as! TableViewCell
        cell.deviceLabel.text = devices[indexPath.row].name
        cell.addressLabel.text = devices[indexPath.row].address
        cell.imgView2.alpha = 0.0
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let cell = tableView.cellForRow(at: indexPath) as! TableViewCell
        cell.imgView2.alpha = 1.0
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            tableView.deselectRow(at: indexPath, animated: true)
        }
        BlueManager.shared.discoveryStop()
        BlueManager.shared.connect(devices[indexPath.row])
    }
    
    // MARK: BlueDelegate Methods
    
    func manager(_ manager: STBlueSDK.BlueManager, discoveringStatus isDiscovering: Bool) {
    }
    
    func manager(_ manager: BlueManager, didDiscover node: Node) {
        DispatchQueue.main.async { [weak self] in

            guard self != nil else { return }
            if !self!.devices.contains(node) {
                self!.devices.append(node)
            }
            self!.tableView.reloadData()

            if let node = BlueManager.shared.discoveredNodes.first,
               let catalog = BlueManager.shared.catalog,
               let firmware = catalog.v2Firmware(with: node.deviceId.longHex,
                                                 firmwareId: UInt32(node.bleFirmwareVersion).longHex) {

                BlueManager.shared.updateDtmi(with: .prod, firmware: firmware) { result, error in

                    Logger.debug(text: "\(result.count)")
                }

            }
        }
    }

    
    func manager(_ manager: BlueManager, didRemoveDiscovered nodes: [Node]) {
        DispatchQueue.main.async { [weak self] in

            guard let self = self else { return }
            self.tableView.reloadData()
        }
    }
    
    func manager(_ manager: BlueManager, didChangeStateFor node: Node) {
        if node.state == .connected {
            DispatchQueue.main.async { [weak self] in
                guard let self = self else { return }
                justConnected()
                parentVC!.box = node
                parentVC!.updateView(true)
                self.dismiss(animated: true)
            }
        }
    }
    
    func manager(_ manager: STBlueSDK.BlueManager, didUpdateValueFor node: STBlueSDK.Node, feature: any STBlueSDK.Feature, sample: (any STBlueSDK.AnyFeatureSample)?) {
    }
    
    func manager(_ manager: STBlueSDK.BlueManager, didReceiveCommandResponseFor node: STBlueSDK.Node, feature: any STBlueSDK.Feature, response: any STBlueSDK.FeatureCommandResponse) {
    }
}
