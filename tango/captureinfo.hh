//
//  captureinfo.hh
//  tango
//
//  Created by Dimitri Racordon on 31.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#pragma once


namespace tango {

    struct Decl;

    /// Stores both the declaration being captured, along with flags that
    /// indicate how it is captured.
    struct CapturedValue {

        CapturedValue(Decl* decl, bool is_noescape = true)
            : decl(decl), is_noescape(is_noescape) {}

        /// Points to the declaration being captured.
        Decl* decl;

        /// Set when a PropertyDecl is captured by a noescape closure.
        bool is_noescape;

    };

} // namespace tango
