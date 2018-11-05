// This file is part of the Acts project.
//
// Copyright (C) 2016-2018 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

<<<<<<< HEAD
#include <algorithm>

namespace Acts {
namespace detail {
  /// The StepperExtensionList allows to add an arbitrary number of step
  /// evaluation algorithms for the RKN4 evaluation. These can be categorised in
  /// two general types:
  /// a) Step evaluation that should not be evaluated along with other
  /// extensions, or at least would overwrite partial results. This means that
  /// in the best case unnecessary/redundant calculation would be performed, in
  /// the worst case the evaluation would go wrong.
=======
namespace Acts {
namespace detail {
  /// The ExtensionList allows to add an arbitrary number of step evaluation
  /// algorithms for the RKN4 evaluation. These can be categorised in two
  /// general
  /// types:
  /// a) Step evaluation that should not be evaluated along other extensions, or
  /// at least would overwrite partial results. This means that in the best case
  /// unnecessary/redundant calculation would be performed, in the worst case
  /// the
  /// evaluation would go wrong.
>>>>>>> Auctioneer setting in EigenStepper added; Covariance test for test cases a&b in unit test added
  /// b) The step evaluation remains untouched and only further calculations are
  /// performed (like additional features or data gathering) that can be treated
  /// as independent of the basic step evaluation in type a). These types can be
  /// added but do not require special treatment in order to not ruin the step
  /// evaluation.
  /// The concept of the auctioneers aims in the first place to judge which
  /// extension of category a) is the one to go. Although every extension can
  /// judge if it is valid based on the data given from the state of stepper,
  /// multiple extensions from type a) could fulfill their dependencies. Since
<<<<<<< HEAD
  /// an extension does not know about other extensions, the decision for the
  /// best extension for the step can only be estimated on a global scope. This
  /// is the job of the auctioneers.
  ///
  /// TODO: An anticipation of an optimal concept of the input (and maybe also
  /// the output) of the call operator of an auctioneer cannot be performed at
  /// the current stage but the concept of passing booblean vectors could be
  /// extended to vectors of ints or doubles (this is the equivalent to a bid
  /// which every extension can make for the upcoming step). At the current
  /// stage, a bid-system would be pure guessing.
=======
  /// an
  /// extension does not know about other extensions, the decision for the best
  /// extension for the step can only be estimated on a global scope. This is
  /// the
  /// job of the auctioneers.
  ///
  /// TODO: An anticipation of an optimal concept of the input (and maybe alos
  /// the
  /// output) of the call operator of an auctioneer cannot be performed at the
  /// current stage but the concept of passing booblean vectors could be
  /// extended
  /// to vectors of ints or doubles (this is the equivalent to a bid which every
  /// extension can make for the upcoming step). At the current stage, a
  /// bid-system would be pure guessing.
>>>>>>> Auctioneer setting in EigenStepper added; Covariance test for test cases a&b in unit test added

  /// @brief Auctioneer that takes all extensions as valid that state to be
  /// valid
  struct VoidAuctioneer
  {
    /// @brief Default constructor
    VoidAuctioneer() = default;

<<<<<<< HEAD
    /// @brief Call operator that returns the list of valid candidates as valids
    ///
    /// @param [in] vCandidates Candidates that are treated as valid extensions
    /// @return The to vCandidates identical list of valid extensions
    template <long unsigned int N>
    std::array<bool, N>
    operator()(std::array<int, N> vCandidates) const
    {
      std::array<bool, N> valids;

      for (unsigned int i = 0; i < vCandidates.size(); i++) {
        valids[i] = (vCandidates[i] > 0) ? true : false;
      }
      return valids;
    }
  };

  /// @brief Auctioneer that states only the first valid extension as indeed
=======
    /// @brief Call operator that just returns the list of candidates as valids
    ///
    /// @param [in] vCandidates Candidates that are treated as valid extensions
    /// @return The to vCandidates identical list of valid extensions
    std::vector<bool>
    operator()(std::vector<bool> vCandidates)
    {
      return std::move(vCandidates);
    }
  };

  /// @brief Auctioneer that states only the first valid extensions as indeed
>>>>>>> Auctioneer setting in EigenStepper added; Covariance test for test cases a&b in unit test added
  /// valid extension
  struct FirstValidAuctioneer
  {
    /// @brief Default constructor
    FirstValidAuctioneer() = default;

    /// @brief Call operator that states the first valid extension as the only
    /// valid extension
    ///
    /// @param [in] vCandidates Candidates for a valid extension
    /// @return List with at most one valid extension
<<<<<<< HEAD
    template <long unsigned int N>
    std::array<bool, N>
    operator()(std::array<int, N> vCandidates) const
    {
      std::array<bool, N> valids = {};

      for (unsigned int i = 0; i < vCandidates.size(); i++) {
        if (vCandidates[i] > 0) {
          valids[i] = true;
          return valids;
        }
      }
      return valids;
    }
  };

  /// @brief Auctioneer that states only the highest bidding extension as indeed
  /// valid extension. If multiple elements have the same int, the first one
  /// with this value is valid.
  struct HighestValidAuctioneer
  {
    /// @brief Default constructor
    HighestValidAuctioneer() = default;

    /// @brief Call operator that states the highest bidding extension as the
    /// only
    /// valid extension
    ///
    /// @param [in] vCandidates Candidates for a valid extension
    /// @return List with at most one valid extension
    template <long unsigned int N>
    std::array<bool, N>
    operator()(std::array<int, N> vCandidates) const
    {
      std::array<bool, N> valids = {};

      auto highscore = std::max_element(vCandidates.begin(), vCandidates.end());
      valids.at(std::distance(vCandidates.begin(), highscore)) = true;

      return valids;
=======
    std::vector<bool>
    operator()(std::vector<bool> vCandidates)
    {
      // Indicator if the first valid was already found
      bool firstValidFound = false;
      //~ for (bool& v : vCandidates) {
      for (unsigned int i = 0; i < vCandidates.size(); i++) {
        // If a valid extensions is already found, set all following to false
        if (firstValidFound) vCandidates[i] = false;
        // If the first valid isn't found yet, toggle the flag on the first
        // found
        else if (vCandidates[i])
          firstValidFound = true;
      }
      return std::move(vCandidates);
>>>>>>> Auctioneer setting in EigenStepper added; Covariance test for test cases a&b in unit test added
    }
  };

}  // namespace detail
}  // namespace Acts