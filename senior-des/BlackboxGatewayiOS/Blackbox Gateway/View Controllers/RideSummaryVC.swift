//
//  RideSummaryVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 4/25/24.
//

import UIKit
import FirebaseAuth
import FirebaseFirestore
import FirebaseStorage
import CoreLocation

class RideSummaryVC: UIViewController {
    
    let db = Firestore.firestore()
    let st = Storage.storage()
    var user: User?
    var testMode = false
    var del: RideSummaryDelegate?
    @IBOutlet weak var dateLabel: UILabel!
    @IBOutlet weak var durationLabel: UILabel!
    @IBOutlet weak var locationLabel: UILabel!
    @IBOutlet weak var fileSizeLabel: UILabel!
    
    var date: String?
    var duration: String?
    var loc: CLLocationCoordinate2D?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.isModalInPresentation = true
        
        guard let u = Auth.auth().currentUser, let d = duration, let da = date, let _ = loc, let _ = del else { self.dismiss(animated: true); return }
        user = u
        
        let splits = da.components(separatedBy: "/")
        dateLabel.text = "\(splits[0]) \(Constants.months[splits[1]] ?? "Month"), \(splits[2])"
        durationLabel.text = d
        // TODO: Dynamically calculate File Size Text
        userFriendlyLoc()
    }
    
    @IBAction func submitData(_ sender: Any) {
        // Currently uploads some fixed data (eg. scooter ID,
        // riding data)
        guard let userID = Auth.auth().currentUser?.uid else { print("error uploading to firestore"); return }
        let curTime = getCurrentTime()
        var dataUp = [String: Any]()
        if !testMode {
            dataUp = ["userID": userID, "time": curTime, "date": date!, "duration": duration!, "storageID": "sens001.csv", "location": ["lat": loc!.latitude, "long": loc!.longitude]]
        } else {
            dataUp = ["userID": userID, "time": curTime, "date": date!, "duration": duration!, "storageID": "sens002.csv", "location": ["lat": loc!.latitude, "long": loc!.longitude], "scores": ["bump": ["percentage": 8.43, "rating": "high"], "fall": ["percentage": 8.57, "rating": "low"], "normal": ["percentage": 80.44, "rating": "medium"], "wobbly": ["percentage": 2.56, "rating": "medium"]]]
        }
//        let localFile = URL(string: "\(FileManager.default.currentDirectoryPath)Blackbox Gateway/sens001.csv")!
//        do {
//            print(try localFile.checkResourceIsReachable())
//        } catch {
//            print("Local File Error")
//        }
        db.collection("Scooters").document("3245").collection("users").document(user!.uid).collection("rides").addDocument(data: dataUp) { err in
            if let _ = err {
                // handle error
                print("error uploading to firestore - \(err!)")
            } else {
                // TODO: Upload dynamically recorded data
//                self.st.reference().child("\(self.user!.uid)/sens001.csv").putFile(from: localFile, metadata: nil) { Storage, err in
//                    if let _ = err {
//                        // handle error
//                        print("error uploading to storage - \(err!)")
//                    } else {
                self.dismiss(animated: true) {
                    self.del!.showSuccess()
                }
//                    }
//                }
                
            }
        }
    }
    
    func userFriendlyLoc() {
        if let lastLocation = loc {
            let geocoder = CLGeocoder()
            geocoder.reverseGeocodeLocation(CLLocation(latitude: lastLocation.latitude, longitude: lastLocation.longitude),
                        completionHandler: { (placemarks, error) in
                if error == nil {
                    let firstLocation = placemarks?[0]
                    self.locationLabel.text = "\( firstLocation!.locality!)"
                } else {
                    // An error occurred during geocoding.
                    self.locationLabel.text = "Undefined"
                }
            })
        } else {
            // No location was available.
            self.locationLabel.text = "Undefined"
        }
    }
    
    func getCurrentTime() -> String {
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "HH:mm"
        return dateFormatter.string(from: Date())
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        self.view.frame = CGRect(x: 0, y: UIScreen.main.bounds.height / 5 * 2.6, width: self.view.bounds.width, height: UIScreen.main.bounds.height / 5 * 2.5)
        self.view.layer.cornerRadius = 20
        self.view.layer.masksToBounds = true
        
    }
}
