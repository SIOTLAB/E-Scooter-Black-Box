//
//  HomeVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 3/21/24.
//

import UIKit
import STCore
import STBlueSDK
import CoreLocation
import JGProgressHUD

class HomeVC: UIViewController, CLLocationManagerDelegate, RideSummaryDelegate {
    
    let locationManager = CLLocationManager()
    
    var curLoc: CLLocationCoordinate2D?
    var locAllowed = false
    var box: Node?
    var buttonSearch = true
    var riding = false
    var timer = Timer()
    var seconds = 0
    
    @IBOutlet weak var statusIcon: UIImageView!
    @IBOutlet weak var statusText: UILabel!
    @IBOutlet weak var searchButton: UIButton!
    @IBOutlet weak var startButton: UIButton!
    @IBOutlet weak var elapsedLabel: UILabel!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.locationManager.requestWhenInUseAuthorization()
        if CLLocationManager.locationServicesEnabled() {
            locationManager.delegate = self
            locationManager.desiredAccuracy = kCLLocationAccuracyNearestTenMeters
            locAllowed = true
        }
        
        startButton.isEnabled = false
        self.performSegue(withIdentifier: "loginScreen", sender: self)
    }
    
    deinit {
        BlueManager.shared.resetDiscovery()
        Logger.debug(text: "DEINIT: \(String(describing: self))")
    }
    
    func updateView(_ connected: Bool) {
        if connected, let _ = box {
            statusText.text = "Connected"
            statusText.textColor = UIColor.systemGreen
            statusIcon.image = UIImage(systemName: "internaldrive")
            statusIcon.tintColor = UIColor.systemGreen
            searchButton.setImage(UIImage(systemName: "slider.horizontal.3"), for: .normal)
            buttonSearch = false
            startButton.isEnabled = true
        }
    }

    @IBAction func searchForDevices(_ sender: UIButton) {
        if buttonSearch {
            self.performSegue(withIdentifier: "deviceList", sender: self)
        } else {
            self.performSegue(withIdentifier: "deviceDetails", sender: self)
        }
    }
    
    @IBAction func startPressed(_ sender: UIButton) {
        if riding {
            timer.invalidate()
            seconds = 0
            startButton.setTitle("Start Ride", for: .normal)
            startButton.tintColor = UIColor.systemGreen
            riding = false
            timer = Timer()
            self.performSegue(withIdentifier: "showSummary", sender: self)
            elapsedLabel.text = "00:00:00"
            locationManager.stopUpdatingLocation()
        } else {
            // start timer, live update label
            elapsedLabel.text = "00:00:00"
            timer = Timer.scheduledTimer(timeInterval: 1, target: self, selector: (#selector(updateTimer)), userInfo: nil, repeats: true)
            startButton.setTitle("End Ride", for: .normal)
            startButton.tintColor = UIColor.systemRed
            riding = true
            if locAllowed {
                locationManager.startUpdatingLocation()
            }
        }
    }
    
    @objc func updateTimer() {
        seconds += 1
        elapsedLabel.text = timeString(time: TimeInterval(seconds))
    }
    
    @IBAction func profileButton(_ sender: UIBarButtonItem) {
        self.performSegue(withIdentifier: "profile", sender: self)
    }
    
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        if segue.identifier == "deviceList", let destVC = segue.destination as? DeviceDiscoveryVC {
            destVC.parentVC = self
        } else if segue.identifier == "deviceDetails", let destVC = segue.destination as? DeviceDetailsVC {
            destVC.device = box
            destVC.f1 = box!.characteristics.allFeatures()[3] // accelerometer
            destVC.f2 = box!.characteristics.allFeatures()[4] // gyroscope
        } else if segue.identifier == "showSummary", let destVC = segue.destination as? RideSummaryVC {
            destVC.duration = elapsedLabel.text!
            destVC.date = Date.getCurrentDate()
            destVC.loc = curLoc ?? nil
            destVC.del = self
        }
    }
    
    func timeString(time: TimeInterval) -> String {
        let hours = Int(time) / 3600
        let minutes = Int(time) / 60 % 60
        let seconds = Int(time) % 60
        return String(format:"%02i:%02i:%02i", hours, minutes, seconds)
    }
    
    // MARK: CLLocationManager Methods
    
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let locValue: CLLocationCoordinate2D = manager.location?.coordinate else { return }
        print("locations = \(locValue.latitude) \(locValue.longitude)")
        curLoc = locValue
    }
    
    func showSuccess() {
        let hud = JGProgressHUD()
        hud.textLabel.text = "Uploading data..."
        hud.show(in: self.view)
        hud.dismiss(afterDelay: 2.0)
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.5) {
            let alert = UIAlertController(title: "E-Scooter Blackbox", message: "Data uploaded successfully!", preferredStyle: UIAlertController.Style.alert)
            alert.addAction(UIAlertAction(title: "Okay", style: UIAlertAction.Style.default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        }
    }
}

// MARK: Date Helper

extension Date {

 static func getCurrentDate() -> String {
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "dd/MM/yyyy"
        return dateFormatter.string(from: Date())
    }
}

protocol RideSummaryDelegate {
    func showSuccess()
}
