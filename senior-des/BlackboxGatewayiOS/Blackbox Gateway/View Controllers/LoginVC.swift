//
//  LoginVC.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 4/5/24.
//

import UIKit
import FirebaseCore
import FirebaseFirestore
import FirebaseAuth
      

class LoginVC: UIViewController, UITextFieldDelegate {
    
    @IBOutlet weak var tf1: UITextField!
    @IBOutlet weak var tf2: UITextField!
    @IBOutlet weak var loginButton: UIButton!
    
    let defaults = UserDefaults.standard
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.isModalInPresentation = true
        tf1.tag = 0
        tf2.tag = 1
        tf1.delegate = self
        tf2.delegate = self
        
        
        if let savedEmail = defaults.object(forKey: "email") as? String, let savedPass = defaults.object(forKey: "pass") as? String {
            loginButton.setTitle("Auto-Logging In...", for: .normal)
            tf1.text = savedEmail
            tf2.text = savedPass
            login()
        } else {
            tf1.becomeFirstResponder()
        }
    }
    
    func login() {
        guard tf1.text != "" && tf2.text != "" else {return}
        Auth.auth().signIn(withEmail: tf1.text!, password: tf2.text!) { [weak self] authResult, error in
            guard self != nil else { return }
            if let _ = error {
                print("Error logging in...")
            } else {
                self!.defaults.set(self!.tf1.text!, forKey: "email")
                self!.defaults.set(self!.tf2.text!, forKey: "pass")
                self!.dismiss(animated: true)
            }
        }
    }
    
    @IBAction func login(_ sender: UIButton) {
        login()
    }
    
    // MARK: UITextField Methods
    
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        if textField.tag == 0 {
            tf2.becomeFirstResponder()
        } else {
            login()
        }
        return true
    }
}
