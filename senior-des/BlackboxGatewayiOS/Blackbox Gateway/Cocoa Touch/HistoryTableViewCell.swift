//
//  HistoryTableViewCell.swift
//  Blackbox Gateway
//
//  Created by Soham Phadke on 4/19/24.
//

import UIKit

class HistoryTableViewCell: UITableViewCell {

    @IBOutlet weak var durationLabel: UILabel!
    @IBOutlet weak var dateLabel: UILabel!
    
    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
    }

    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)

        // Configure the view for the selected state
    }
    
}
