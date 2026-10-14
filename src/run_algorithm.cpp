#include <bsp/Graph.hpp>
#include <bsp/run.hpp>
#include <bsp/thermo_sp.hpp>
#include <bsp/thermo_policy.hpp>
#include <bsp/tsallis.hpp>
#include <bsp/rsb2.hpp>
#include <bsp/sp2.hpp>

// Shared orchestration: preserve the executable's update and RNG order.
bool bsp::run_algorithm(Graph &G, std::vector<Vertex> &V) {
    V.resize(G.N());
    for(unsigned i=0;i<G.N();++i) {
        V[i]._vertex=i+1;
        V[i]._vertex_lli=i+1;
    }
    G.get_V(V);
    G.split_and_collect_information();
    if(g_r_bsp!=0.) BSP_INFO<<"START BSP WITH r="<<g_r_bsp<<":"<<endl;
    else BSP_INFO<<"START SID:"<<endl;
    BSP_INFO<<"Decimation scorer: "<<bsp_scorer_name()<<endl;
    if(g_T_cav!=0.) BSP_INFO<<"ThermoSP cavity temperature: T="<<g_T_cav<<endl;
    if(g_T_act!=0.) BSP_INFO<<"Gibbs action temperature: T_act="<<g_T_act<<endl;
    if(g_rsb_m!=0.) BSP_INFO<<"1RSB cluster reweighting: m="<<g_rsb_m<<" (MMW closure, effective gamma="<<g_gamma_eff<<")"<<endl;
    if(g_rsb_gamma!=0.) BSP_INFO<<"1RSB unfrozen bias: gamma="<<g_rsb_gamma<<endl;
    if(g_tsallis_kappa!=0.) BSP_INFO<<"Tsallis outer loop: kappa="<<g_tsallis_kappa<<" base y="<<g_tsallis_y<<endl;
    if(g_fe_backtrack) BSP_INFO<<"Free-energy backtracking on, bt-cost="<<g_bt_cost<<endl;
    if(g_lookahead_k>1 || g_corr_batch || g_adaptive_r || g_frac!=frac || g_dynamic_i)
        BSP_INFO<<"profile controls: lookahead="<<g_lookahead_k<<" corr_batch="<<(g_corr_batch?1:0)
                <<" adaptive_r="<<(g_adaptive_r?1:0)<<" frac="<<g_frac<<" dynamic_i="<<(g_dynamic_i?1:0)<<endl;
    G.unit_propagation();
    while(true) {
        check_cancelled();
        G.convergence_messages();
        G.surveys();
        G.diag_step();
        if(g_rsb2_stab || g_rsb2_x>0.) rsb2_step(G);
        if(g_sp2_mu2>0.) sp2_step(G);
        if(g_tsallis_kappa!=0.) tsallis_update(G.energy,G.N_t());
        if(!G.fl_bsp) {
            BSP_INFO<<G;
            G.save();
            if(G.complexity==0.) break;
            if(G.complexity<-2.) {
                BSP_ERROR<<"Negative complexity: I am not able to find a solution ;("<<endl;
                BSP_ERROR<<"I am sorry I quit"<<endl;
                throw HeuristicFailure("negative complexity");
            }
            G.sort_V_Dec_move();
            G.choose_var_to_fix_and_clean();
        } else G.backtrack();
    }
    if(!run_context()) G.print_on_file_residual_formula();
    bool solved=G.WalkSAT();
    if(solved && !run_context()) G.print();
    return solved;
}
