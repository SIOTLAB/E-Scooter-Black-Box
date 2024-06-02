//
//  ProfileVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 4/5/24.
//

import UIKit
import FirebaseAuth
import FirebaseFirestore

class ProfileVC: UIViewController {
    
    @IBOutlet weak var nameLabel: UILabel!
    @IBOutlet weak var emailLabel: UILabel!
    
    let db = Firestore.firestore()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let user = Auth.auth().currentUser else {return}
        
        emailLabel.text = user.email
        
        db.collection("userData").document(user.uid).getDocument { docSnap, error in
            if error != nil || docSnap == nil {
                print("Error getting name data")
            } else {
                self.nameLabel.text = docSnap!.data()!["name"] as? String ?? "ERROR"
                self.emailLabel.text = user.email ?? "email"
            }
        }
    }
}
