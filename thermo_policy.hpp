//
//  thermo_policy.hpp
//  BSP
//
//  Thermodynamic Survey Propagation: Gibbs decision policy (pi_T of the note).
//

#ifndef THERMO_POLICY_HPP
#define THERMO_POLICY_HPP

/*Action temperature of the Gibbs decimation policy. T_act=0 keeps the hard
 assignment rule of the legacy code (true iff sT>sF). Set with --act-temp=T.
 The policy runs on the free energies of the deformed messages, so it needs
 --cav-temp>0.*/
extern double g_T_act;

/*Gibbs draw of one assignment. The true direction carries the weight
 e^(-phi_plus/T_act) and the false direction e^(-phi_minus/T_act), where the
 two free energies come from the deformed cavity biases. At T_act=0 the
 direction with the smaller free energy wins, and ties give false like the
 legacy rule. Returns true for x_i=true.*/
bool thermo_gibbs_direction(double phi_plus, double phi_minus, double T_act);

#endif /* THERMO_POLICY_HPP */
