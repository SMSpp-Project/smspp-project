/*--------------------------------------------------------------------------*/
/*---------------- File UCScenarioReductionTest.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * UC-specific implementation of AbstractScenarioReductionTest.
 *
 * Implements the two pure virtual methods added by the refactoring:
 *  - create_srb()                  builds a ScenarioReductionBlock
 *  - build_tssb_for_current_pool() builds a TSSB from the current pool
 *
 * Everything else (caching, VPI, warmstart, print_results) is inherited
 * from AbstractScenarioReductionTest.
 *
 * \author Benoît Tran \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Benoît Tran
 */
/*--------------------------------------------------------------------------*/

#ifndef __UCScenarioReductionTest
#define __UCScenarioReductionTest

#include "AbstractScenarioReductionTest.h"
#include "CSSCScenarioReductionSolver.h"
#include "IntermittentUnitBlock.h"
#include "ThermalUnitBlock.h"
#include "UCBlock.h"

#include <memory>
#include <string>
#include <vector>

namespace ScenarioReductionTesting {

 using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS UCScenarioReductionTest ---------------------*/
/*--------------------------------------------------------------------------*/

 class UCScenarioReductionTest : public AbstractScenarioReductionTest {

 public:

  UCScenarioReductionTest()  = default;
  ~UCScenarioReductionTest() override = default;

 protected:

  // ===================== Pure virtuals from AbstractSRT ==================

  /** Load base UCBlock instance. Sets base_block and stochastic_block */
  void load_problem_instance( const std::string & path ) override;

  /** Build a fully configured ScenarioReductionBlock.
   *
   *  Always sets: scenario_generator, sub_problem_block (synthetic CFLB).
   *  For CSSC also sets: stochastic_block (TSSB), scenario_applicator
   *
   *  Owns the synthetic CFLB and (for CSSC) the TSSB through member
   *  variables so they stay alive as long as the SRB is alive
   */
  std::unique_ptr< ScenarioReductionBlock >
  create_srb( int K , const std::string & method ) override;

  /** Build a TSSB from the base UCBlock + current scenario_set pool */
  std::unique_ptr< TwoStageStochasticBlock >
  build_tssb_for_current_pool( const std::string & tmp ) override;

  /** @return "UC" */
  std::string get_problem_type() const override { return "UC"; }

  /** UC CSSC: uses CSSCScenarioReductionSolver with a UC-specific
   *  VarExtractor lambda that identifies commitment variables */
  void run_cssc( ScenarioReductionBlock * srb ,
                 BlockSolverConfig      * bsc ,
                 int                      K  ) override;

  /** @return "../scenarios/UCBlock/" */
  std::string get_scenarios_directory() const override {
   return "../scenarios/UCBlock/";
  }

 private:

  /** Which uncertain parameters appear in the scenario vectors */
  enum class UncertaintyType { kDemandOnly , kRenewableOnly , kBoth };

  /** Detect uncertainty type from scenario vector size */
  UncertaintyType infer_uncertainty_type( size_t scenario_dim ) const;

  /** Indices of IntermittentUnitBlock children of the loaded UCBlock */
  static std::vector< Index > get_intermittent_indices( UCBlock * uc );

  /** Internal build_tssb: takes an explicit DSS (for create_srb CSSC path) */
  std::unique_ptr< TwoStageStochasticBlock >
  build_tssb( UCBlock * uc ,
              DiscreteScenarioSet * dss ,
              const std::string & tmp ,
              UncertaintyType utype ) const;

  // State set by load_problem_instance() 
  UncertaintyType             uncertainty_type_ = UncertaintyType::kRenewableOnly;
  std::vector< Index >        intermittent_units_;
  size_t                      num_time_periods_ = 0;
  size_t                      num_nodes_        = 0;
  std::string                 instance_file_path_;  // for EC Block copy workaround

  // Owned objects that must outlive the SRB (for CSSC) 
  // Reset at the start of each create_srb() call
  std::unique_ptr< TwoStageStochasticBlock >          srb_tssb_;

 };  // class UCScenarioReductionTest

}  // namespace ScenarioReductionTesting

#endif /* __UCScenarioReductionTest */

/*--------------------------------------------------------------------------*/
/*---------------- End File UCScenarioReductionTest.h ---------------------*/
/*--------------------------------------------------------------------------*/