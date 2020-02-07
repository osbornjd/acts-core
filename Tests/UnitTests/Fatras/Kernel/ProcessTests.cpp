// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include <array>
#include <random>

#include "Acts/Material/MaterialProperties.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/Utilities/Units.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Kernel/Process.hpp"

using namespace Acts::UnitLiterals;
using namespace ActsFatras;

namespace {

/// A process leaves the particle as-is and creates four daughter particles
/// with momenta 1,2,3,4GeV.
struct MakeChildren {
  template <typename generator_t>
  std::array<ActsFatras::Particle, 4> operator()(
      generator_t &, const Acts::MaterialProperties &,
      ActsFatras::Particle &) const {
    // create daughter particles
    return {
        Particle().setMomentum(1_GeV),
        Particle().setMomentum(2_GeV),
        Particle().setMomentum(3_GeV),
        Particle().setMomentum(4_GeV),
    };
  }
};

/// Select particles with momenta equal or above 10GeV.
struct HighP {
  double minP = 10_GeV;

  bool operator()(const ActsFatras::Particle &particle) const {
    return (minP <= particle.momentum());
  }
};

struct Fixture {
  std::default_random_engine generator;
  Acts::MaterialProperties slab{Acts::Test::makeBeryllium(), 1_mm};
  Particle parent = Particle().setMomentum(10_GeV);
  std::vector<Particle> children;
};

}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasProcess)

BOOST_AUTO_TEST_CASE(NoSelectors) {
  Fixture f;
  Process<MakeChildren> process;

  // process should not abort
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 4u);
}

BOOST_AUTO_TEST_CASE(WithInputSelector) {
  Fixture f;
  Process<MakeChildren, HighP> process;
  process.selectInput.minP = 10_GeV;

  // above threshold should not abort
  f.parent.setMomentum(20_GeV);
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 4u);
  // on threshold should still not abort
  f.parent.setMomentum(10_GeV);
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 8u);
  // below threshold should abort and not run the process at all
  f.parent.setMomentum(2_GeV);
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  // process did not run -> no new children
  BOOST_TEST(f.children.size() == 8u);
}

BOOST_AUTO_TEST_CASE(WithOutputSelector) {
  Fixture f;
  Process<MakeChildren, EveryParticle, HighP> process;
  process.selectOutput.minP = 10_GeV;

  // above threshold should not abort
  f.parent.setMomentum(20_GeV);
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 4u);
  // on threshold should still not abort
  f.parent.setMomentum(10_GeV);
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 8u);
  // below threshold should abort but only after running the process
  f.parent.setMomentum(2_GeV);
  BOOST_TEST(process(f.generator, f.slab, f.parent, f.children));
  // process did still run -> new children
  BOOST_TEST(f.children.size() == 12u);
}

BOOST_AUTO_TEST_CASE(WithChildSelector) {
  Fixture f;
  Process<MakeChildren, EveryParticle, EveryParticle, HighP> process;
  process.selectChild.minP = 10_GeV;

  // all process should not abort regardless of child selection
  // select no daughters
  process.selectChild.minP = 5_GeV;
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 0u);
  // select highest daughter
  process.selectChild.minP = 3.5_GeV;
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 1u);
  // select all but the lowest daughter
  process.selectChild.minP = 1.5_GeV;
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 4u);
  // select all daughters
  process.selectChild.minP = 0.5_GeV;
  BOOST_TEST(not process(f.generator, f.slab, f.parent, f.children));
  BOOST_TEST(f.children.size() == 8u);
}

BOOST_AUTO_TEST_SUITE_END()
