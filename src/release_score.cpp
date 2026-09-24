//
//  release_score.cpp
//  BSP
//
//  Release score I(k) of a fixed variable from the current SP fixed point.
//

#include <bsp/Graph.hpp>
#include <bsp/thermo_sp.hpp>

#include <cmath>
#include <vector>

using namespace std;

/*I(k) = 1 - w_k^{-s_k} of a fixed variable k (eq. Ik of paper/sections/response.tex),
 the fraction of the clusters of the released formula in which k is compatible
 with its value s_k. The star of k has two groups:
   against: the unsatisfied clauses, where the literal of k is false;
   toward:  the clauses satisfied by k alone, which come back on release.
 A clause satisfied by another fixed variable sends no warning. Each other
 clause a sends eta_{a->k} = prod_j nu_{j->a} over its free literals j, from
 the current products. In a clause satisfied by k alone, the slots of a in
 the free variables are 0, so the cavity of j needs no division. The slots of
 k itself are not used: Clause::clean sets them to 0 or 1.
 With toward=false the toward group is dropped, which is the older score B0.*/
double Graph::release_I(Vertex *k, bool toward) {
    const bool thermo=(g_T_cav>0. || g_gamma_eff!=0.);
    double one=1.;
    vector<double> eta_toward, eta_against;
    double A0=1., B0=1.;
    for (unsigned j=0; j<k->_I_am_in_cl_at_init.size(); ++j) {
        Clause &c=_cl[*real(k->_I_am_in_cl_at_init[j])];
        unsigned pos=*imag(k->_I_am_in_cl_at_init[j]);
        bool against=c._I_am_in_list_unsat;
        if (!against) {
            if (!toward) continue;
            bool other=false;
            for (unsigned l=0; l<c._size_cl_init && !other; ++l)
                other=(l!=pos && c._vb[l]);
            if (other) continue;
        }
        double eta=1.;
        for (unsigned l=0; l<c._size_cl_init; ++l) {
            if (!c._go_forward[l]) continue;
            _prod_S=_Pr_S(c.v_V[l],c.v_lit[l],against?c.div_s[l]:one);
            _prod_U=_Pr_U(c.v_V[l],c.v_lit[l]);
            if (thermo) {
                ThermoStar st=c.v_V[l]->cavity_star(c.v_lit[l],c.v_survey_cl_to_i[l],_prod_S,_prod_U,g_T_cav,g_gamma_eff);
                eta*=st.pi_u/st.z;
            } else {
                eta*=__pu()/__norm();
            }
        }
        if (against) {
            eta_against.push_back(eta);
            B0*=1.-eta;
        } else {
            eta_toward.push_back(eta);
            A0*=1.-eta;
        }
    }
    ThermoStar st=thermo_star(eta_toward.data(),eta_toward.size(),
                              eta_against.data(),eta_against.size(),
                              A0,B0,g_T_cav,g_gamma_eff);
    double I=(st.z>0.)?(st.pi_s+st.pi_0)/st.z:B0;/*z=0 only if both groups force k*/
    return (I>1e-300)?I:1e-300;
}
