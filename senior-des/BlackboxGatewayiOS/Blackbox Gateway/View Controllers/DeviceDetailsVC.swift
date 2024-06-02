//
//  DeviceDetailsVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 3/22/24.
//

import UIKit
import STBlueSDK
import STCore
import DGCharts

class DeviceDetailsVC: UIViewController, BlueDelegate {
    
    var device: Node?
    var f1: Feature?
    var f2: Feature?
    var base: BaseNodeViewController?
    var counter = 0.0

    var accelXData = [ChartDataEntry]()
    var accelYData = [ChartDataEntry]()
    var accelZData = [ChartDataEntry]()
    var gyroXData = [ChartDataEntry]()
    var gyroYData = [ChartDataEntry]()
    var gyroZData = [ChartDataEntry]()
    
    @IBOutlet weak var mainTitle: UILabel!
    @IBOutlet weak var deviceLabel: UILabel!
    @IBOutlet weak var macLabel: UILabel!
    @IBOutlet weak var accelX: UILabel!
    @IBOutlet weak var accelY: UILabel!
    @IBOutlet weak var accelZ: UILabel!
    @IBOutlet weak var gyroX: UILabel!
    @IBOutlet weak var gyroY: UILabel!
    @IBOutlet weak var gyroZ: UILabel!
    @IBOutlet weak var graphView1: LineChartView!
    @IBOutlet weak var graphView2: LineChartView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        mainTitle.text = "\(device!.name ?? "Device") Details"
        macLabel.text = "\(device!.address ?? "MAC ADDRESS")"
        
        graphView1.chartDescription.text = "Accelerometer Data"
        graphView2.chartDescription.text = "Gyroscope Data"
        
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        BlueManager.shared.addDelegate(self)
//        BlueManager.shared.enableNotifications(for: device!, feature: f1!)
        BlueManager.shared.enableNotifications(for: device!, feature: f2!)
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        
//        BlueManager.shared.disableNotifications(for: device!, feature: f1!)
        BlueManager.shared.disableNotifications(for: device!, feature: f2!)
        BlueManager.shared.removeDelegate(self)
        Logger.debug(text: "DEINIT: \(String(describing: self))")
    }
    
    // MARK: BlueDelegate Methods
    
    func manager(_ manager: STBlueSDK.BlueManager, discoveringStatus isDiscovering: Bool) { }
    
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
            
            let description = feature.description(with: sample)
//            print(description)
            counter += 0.1
            
            if description.contains("AccelerationFeature") {
                let splits = description.components(separatedBy: ":")
                
                gyroX.text = "\(splits[4].components(separatedBy: "\n")[0])"
                gyroY.text = "\(splits[5].components(separatedBy: "\n")[0])"
                gyroZ.text = "\(splits[6].components(separatedBy: "\n")[0])"
                
                gyroXData.append(ChartDataEntry(x: counter, y: Double(String(gyroX.text!.dropFirst())) ?? 0.0))
                gyroYData.append(ChartDataEntry(x: counter, y: Double(String(gyroY.text!.dropFirst())) ?? 0.0))
                gyroZData.append(ChartDataEntry(x: counter, y: Double(String(gyroZ.text!.dropFirst())) ?? 0.0))
                let chartDSx = LineChartDataSet(entries: gyroXData, label: "gyroX")
                let chartDSy = LineChartDataSet(entries: gyroYData, label: "gyroY")
                let chartDSz = LineChartDataSet(entries: gyroZData, label: "gyroZ")
                let chartData = LineChartData(dataSets: [chartDSx, chartDSy, chartDSz])
                chartDSx.colors = [UIColor.systemRed]
                chartDSy.colors = [UIColor.systemBlue]
                chartDSz.colors = [UIColor.systemGreen]
                chartDSy.drawCirclesEnabled = false
                chartDSz.drawCirclesEnabled = false
                chartDSx.drawCirclesEnabled = false
                chartDSx.drawValuesEnabled = false
                chartDSy.drawValuesEnabled = false
                chartDSz.drawValuesEnabled = false
                graphView2.data = chartData
                
            } else if description.contains("GyroscopeFeature") {
                let splits = description.components(separatedBy: ":")
                
                accelX.text = "\(splits[4].components(separatedBy: "\n")[0])"
                accelY.text = "\(splits[5].components(separatedBy: "\n")[0])"
                accelZ.text = "\(splits[6].components(separatedBy: "\n")[0])"
                
                accelXData.append(ChartDataEntry(x: counter, y: Double(String(accelX.text!.dropFirst())) ?? 0.0))
                accelYData.append(ChartDataEntry(x: counter, y: Double(String(accelY.text!.dropFirst())) ?? 0.0))
                accelZData.append(ChartDataEntry(x: counter, y: Double(String(accelZ.text!.dropFirst())) ?? 0.0))
                let chartDSx = LineChartDataSet(entries: accelXData, label: "AccelX")
                let chartDSy = LineChartDataSet(entries: accelYData, label: "AccelY")
                let chartDSz = LineChartDataSet(entries: accelZData, label: "AccelZ")
                let chartData = LineChartData(dataSets: [chartDSx, chartDSy, chartDSz])
                chartDSx.colors = [UIColor.systemRed]
                chartDSy.colors = [UIColor.systemBlue]
                chartDSz.colors = [UIColor.systemGreen]
                chartDSy.drawCirclesEnabled = false
                chartDSz.drawCirclesEnabled = false
                chartDSx.drawCirclesEnabled = false
                graphView1.data = chartData
            }
        }
    }
}
