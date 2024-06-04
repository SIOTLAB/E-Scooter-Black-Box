//
//  BaseNodeViewController.swift
//
//  Copyright (c) 2022 STMicroelectronics.
//  All rights reserved.
//
//  This software is licensed under terms that can be found in the LICENSE file in
//  the root directory of this software component.
//  If no LICENSE file comes with this software, it is provided AS-IS.
//

import UIKit
import STBlueSDK
import STCore

class BaseNodeViewController: BlueDelegate {
    func manager(_ manager: STBlueSDK.BlueManager, discoveringStatus isDiscovering: Bool) { }
    
    var accelTextView: UILabel?
    var gyroTextView: UILabel?
    var node: Node
    var accel: Feature?
    var gyro: Feature?

    init(with node: Node, tv: UILabel, tv2: UILabel, acc: Feature, gyr: Feature) {
        self.node = node
        self.accelTextView = tv
        self.gyroTextView = tv2
        self.accel = acc
        self.gyro = gyr
        BlueManager.shared.addDelegate(self)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    deinit {
        BlueManager.shared.removeDelegate(self)
        Logger.debug(text: "DEINIT: \(String(describing: self))")
    }

    func manager(_ manager: BlueManager, didDiscover node: Node) {

    }

    func manager(_ manager: BlueManager, didRemoveDiscovered nodes: [Node]) {
        
    }

    func manager(_ manager: BlueManager, didChangeStateFor node: Node) {

    }

    func manager(_ manager: BlueManager, didReceiveCommandResponseFor node: Node, feature: Feature, response: FeatureCommandResponse) {
        Logger.debug(text: "\(response.description)")
    }
    
    func manager(_ manager: BlueManager, didUpdateValueFor node: Node, feature: Feature, sample: AnyFeatureSample?) {
        DispatchQueue.main.async { [weak self] in
            
            guard let self = self else { return }
            if !(type(of: self.accel!) == type(of: feature) &&
                feature.type.mask == self.accel!.type.mask) && !(type(of: self.gyro!) == type(of: feature) &&
                                                             feature.type.mask == self.gyro!.type.mask) {
                return
            }
            
            let description = feature.description(with: sample)
            
            Logger.debug(text: "\(description)")
//            self.textView.text = description
        }
    }
}
