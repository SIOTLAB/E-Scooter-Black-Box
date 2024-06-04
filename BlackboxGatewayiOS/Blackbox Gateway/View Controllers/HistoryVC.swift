//
//  HistoryVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 4/18/24.
//

import UIKit
import FirebaseCore
import FirebaseAuth
import FirebaseFirestore
import FirebaseStorage

class HistoryVC: UIViewController, UITableViewDelegate, UITableViewDataSource {
    
    @IBOutlet weak var tableView: UITableView!
    
    let storage = Storage.storage()
    let db = Firestore.firestore()
    var records: [(date: String, duration: String, storageID: String)] = []
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let user = Auth.auth().currentUser else {return}
        
        tableView.delegate = self
        tableView.dataSource = self
        tableView.rowHeight = 70
        tableView.register(UINib(nibName: "HistoryTableViewCell", bundle: nil), forCellReuseIdentifier: "reusableHistoryCell")
        
        // TODO: Currently scooter # fixed, should be dynamic depending on SensorTileBox ID
        db.collection("Scooters").document("3245").collection("users").document(user.uid).collection("rides").getDocuments() { docSnap, error in
            if error != nil || docSnap == nil {
                print("Error getting name data")
            } else {
                self.loadRides(docSnap!.documents)
            }
        }
    }
    
    func loadRides(_ collection: [QueryDocumentSnapshot]) {
        for doc in collection {
            if let date = doc.data()["date"] as? String, let duration = doc.data()["duration"] as? String, let storageID = doc.data()["storageID"] as? String {
                records.append((date: date, duration: duration, storageID: storageID))
            }
        }
        tableView.reloadData()
    }
    
    @IBAction func profile(_ sender: Any) {
        self.performSegue(withIdentifier: "profile", sender: self)
    }
    
    // MARK: UITableView Methods
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            tableView.deselectRow(at: indexPath, animated: true)
        }
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return records.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "reusableHistoryCell") as! HistoryTableViewCell
        cell.dateLabel.text = records[indexPath.row].date
        cell.durationLabel.text = records[indexPath.row].duration
        
        return cell
    }
}
