// Contributed by Jose Renau
//                Basilio Fraguela
//                James Tuck
//                Milos Prvulovic
//                Luis Ceze
//                Islam Atta (IBM)
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#include <fstream>
#include <math.h>
#include <algorithm>
#include <iterator>
#include <numeric>

#include "SescConf.h"

#include "OoOProcessor.h"
#include "EmuSampler.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "TaskHandler.h"
#include "FastQueue.h"
#include "MemRequest.h"

//#define ESESC_TRACE

/* }}} */

//#define ESESC_CODEPROFILE
#define ESESC_BRANCHPROFILE

// FIXME: to avoid deadlock, prealloc n to the n oldest instructions
//#define LATE_ALLOC_REGISTER
extern "C" uint64_t esesc_mem_read(uint64_t addr);

OoOProcessor::OoOProcessor(GMemorySystem *gm, CPU_t i)
    /* constructor {{{1 */
    : GOoOProcessor(gm, i)
    , MemoryReplay(SescConf->getBool("cpusimu", "MemoryReplay", i))
#ifdef ENABLE_LDBP
    , BTT_SIZE(SescConf->getInt("cpusimu", "btt_size", i))
    , MAX_TRIG_DIST(SescConf->getInt("cpusimu", "max_trig_dist", i))
    , ldbp_power_mode_cycles("P(%d)_ldbp_power_mode_cycles", i)
    , ldbp_power_save_cycles("P(%d)_ldbp_power_save_cycles", i)
    , num_loads("P(%d)_num_loads", i)
    , num_loads_ldbp("P(%d)_num_loads_ldbp", i)
    , num_loads_non_ldbp("P(%d)_num_loads_non_ldbp", i)
    , num_ld_conf("P(%d)_num_ld_conf", i)
    , num_ld_conf_ldbp("P(%d)_num_ld_conf_ldbp", i)
    , num_ld_conf_non_ldbp("P(%d)_num_ld_conf_non_ldbp", i)
    , num_ld_data_conf("P(%d)_num_ld_data_conf", i)
    , num_ld_data_conf_ldbp("P(%d)_num_ld_data_conf_ldbp", i)
    , num_ld_data_conf_non_ldbp("P(%d)_num_ld_data_conf_non_ldbp", i)
    , num_ldbr("P(%d)_num_ldbr", i)
    , num_non_ldbr("P(%d)_num_non_ldbr", i)
    //, num_non_ldbr("P(%d)_num_non_ldbr", i)
    , num_bad_tage_br("P(%d)_num_bad_tage_br", i)
    , num_br("P(%d)_num_br", i)
    , num_chain_child("P(%d)_num_chain_child", i)
    , num_br_1_src("P(%d)_num_br_1_src", i)
    , num_br_2_src("P(%d)_num_br_2_src", i)
    , num_br_trivial("P(%d)_num_br_trivial", i)
    , num_br_trivial_ldbp("P(%d)_num_br_trivial_ldbp", i)
    , num_br_trivial_non_ldbp("P(%d)_num_br_trivial_non_ldbp", i)
    , num_br_trivial_x_ld("P(%d)_num_br_trivial_x_ld", i)
    , num_br_simple("P(%d)_num_br_simple", i)
    , num_br_simple_ldbp("P(%d)_num_br_simple_ldbp", i)
    , num_br_simple_non_ldbp("P(%d)_num_br_simple_non_ldbp", i)
    , num_br_simple_x_ld("P(%d)_num_br_simple_x_ld", i)
    , num_br_excess("P(%d)_num_br_excess", i)
    , num_br_excess_ldbp("P(%d)_num_br_excess_ldbp", i)
    , num_br_excess_non_ldbp("P(%d)_num_br_excess_non_ldbp", i)
    , num_br_excess_x_ld("P(%d)_num_br_excess_x_ld", i)
    , num_br_chain("P(%d)_num_br_chain", i)
    , num_br_chain_ldbp("P(%d)_num_br_chain_ldbp", i)
    , num_br_chain_non_ldbp("P(%d)_num_br_chain_non_ldbp", i)
    , num_br_chain_x_ld("P(%d)_num_br_chain_x_ld", i)
    , num_br_chain_x_op("P(%d)_num_br_chain_x_op", i)
    , num_br_complex("P(%d)_num_br_complex", i)
    , num_ldbr_2("P(%d)_num_ldbr_2", i)
    , num_ldbr_3("P(%d)_num_ldbr_3", i)
    , num_ldbr_4("P(%d)_num_ldbr_4", i)
    , num_ldbr_12("P(%d)_num_ldbr_12", i)
    , num_ldbr_22("P(%d)_num_ldbr_22", i)
    , num_ldbr_23("P(%d)_num_ldbr_23", i)
    , num_ldbr_24("P(%d)_num_ldbr_24", i)
    , num_ldbr_32("P(%d)_num_ldbr_32", i)
    , num_ldbr_33("P(%d)_num_ldbr_33", i)
    , num_ldbr_34("P(%d)_num_ldbr_34", i)
    , num_ldbr_44("P(%d)_num_ldbr_44", i)
    , num_ldbr_222("P(%d)_num_ldbr_222", i)
    , num_ldbr_223("P(%d)_num_ldbr_223", i)
    , num_ldbr_233("P(%d)_num_ldbr_233", i)
    , num_ldbr_423("P(%d)_num_ldbr_423", i)
    , num_ldbr_444("P(%d)_num_ldbr_444", i)
    , num_ldbr_2222("P(%d)_num_ldbr_2222", i)
    , num_ldbr_2223("P(%d)_num_ldbr_2223", i)
    , num_ldbr_2232("P(%d)_num_ldbr_2232", i)
    , num_ldbr_2322("P(%d)_num_ldbr_2322", i)
    , num_ldbr_2233("P(%d)_num_ldbr_2233", i)
    , num_ldbr_0("P(%d)_num_ldbr_0", i)
    , num_ldbr_6L1("P(%d)_num_ldbr_6L1", i)
    , num_ldbr_6L2_3("P(%d)_num_ldbr_6L2_3", i)
    //, num_ldbr_6L4("P(%d)_num_ldbr_6L4", i)
    , num_ldbr_7L1("P(%d)_num_ldbr_7L1", i)
    , num_ldbr_7L2_3("P(%d)_num_ldbr_7L2_3", i)
    , num_ldbr_7L4("P(%d)_num_ldbr_7L4", i)
    , num_ldbr_8L2_3("P(%d)_num_ldbr_8L2_3", i)
    , num_ldbr_8L4("P(%d)_num_ldbr_8L4", i)
    , num_ldbr_others("P(%d)_num_ldbr_others", i)
#endif
    , RetireDelay(SescConf->getInt("cpusimu", "RetireDelay", i))
    , IFID(i, gm)
    , pipeQ(i)
    , lsq(i,
          SescConf->checkInt("cpusimu", "maxLSQ", i) ? SescConf->getInt("cpusimu", "maxLSQ", i) : 32768) // 32K (unlimited or fix)
    , retire_lock_checkCB(this)
    , clusterManager(gm, i, this)
    , avgFetchWidth("P(%d)_avgFetchWidth", i)
#ifdef TRACK_TIMELEAK
    , avgPNRHitLoadSpec("P(%d)_avgPNRHitLoadSpec", i)
    , avgPNRMissLoadSpec("P(%d)_avgPNRMissLoadSpec", i)
#endif
#ifdef TRACK_FORWARDING
    , avgNumSrc("P(%d)_avgNumSrc", i)
    , avgNumDep("P(%d)_avgNumDep", i)
    , fwd0done0("P(%d)_fwd0done0", i)
    , fwd1done0("P(%d)_fwd1done0", i)
    , fwd1done1("P(%d)_fwd1done1", i)
    , fwd2done0("P(%d)_fwd2done0", i)
    , fwd2done1("P(%d)_fwd2done1", i)
    , fwd2done2("P(%d)_fwd2done2", i)
#endif
    , codeProfile("P(%d)_prof", i) {
  bzero(RAT, sizeof(DInst *) * LREG_MAX);
  bzero(serializeRAT, sizeof(DInst *) * LREG_MAX);
#ifdef TRACK_FORWARDING
  bzero(fwdDone, sizeof(Time_t) * LREG_MAX);
#endif

  spaceInInstQueue = InstQueueSize;

  codeProfile_trigger = 0;

  nTotalRegs = SescConf->getInt("cpusimu", "nTotalRegs", gm->getCoreId());
  if(nTotalRegs == 0)
    nTotalRegs = 1024 * 1024 * 1024; // Unlimited :)

  busy             = false;
  flushing         = false;
  replayRecovering = false;
  getStatsFlag     = false;
  replayID         = 0;

  last_state.dinst_ID = 0xdeadbeef;

  if(SescConf->checkInt("cpusimu", "serialize", i))
    serialize = SescConf->getInt("cpusimu", "serialize", i);
  else
    serialize = 0;

  serialize_level       = 2; // 0 full, 1 all ld, 2 same reg
  serialize_for         = 0;
  last_serialized       = 0;
  last_serializedST     = 0;
  forwardProg_threshold = 200;

#ifdef ENABLE_LDBP
  DL1 = gm->getDL1();
  power_save_mode_ctr = 0;
  power_clock         = 0;
  tmp_power_clock     = 0;
  ldbp_curr_addr      = 0;
  ldbp_delta          = 0;
  inflight_branch     = 0;
  ldbp_brpc           = 0;
  ldbp_ldpc           = 0;
#endif
  if(SescConf->checkBool("cpusimu", "scooreMemory", gm->getCoreId()))
    scooreMemory = SescConf->getBool("cpusimu", "scooreMemory", gm->getCoreId());
  else
    scooreMemory = false;
}
/* }}} */

OoOProcessor::~OoOProcessor()
/* destructor {{{1 */
{
  // Nothing to do
}
/* }}} */

void OoOProcessor::fetch(FlowID fid)
/* fetch {{{1 */
{
  I(fid == cpu_id);
  I(active);
  I(eint);

  if(IFID.isBlocked()) {
    //    I(0);
    busy = true;
  } else {
    IBucket *bucket = pipeQ.pipeLine.newItem();
    if(bucket) {
      IFID.fetch(bucket, eint, fid);
      if(!bucket->empty()) {
        avgFetchWidth.sample(bucket->size(), bucket->top()->getStatsFlag());
        busy = true;
      }
    }
  }
}
/* }}} */

bool OoOProcessor::advance_clock(FlowID fid)
/* Full execution: fetch|rename|retire {{{1 */
{

  if(!active) {
    // time to remove from the running queue
    // TaskHandler::removeFromRunning(cpu_id);
    return false;
  }

  fetch(fid);

  if(!ROB.empty()) {
    // Else use last time
    getStatsFlag = ROB.top()->getStatsFlag();
  }

  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  if(!busy)
    return false;

  if(unlikely(throttlingRatio > 1)) {
    throttling_cntr++;

    uint32_t skip = (uint32_t)ceil(throttlingRatio / getTurboRatio());

    if(throttling_cntr < skip) {
      return true;
    }
    throttling_cntr = 1;
  }

  // ID Stage (insert to instQueue)
  if(spaceInInstQueue >= FetchWidth) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if(bucket) {
      I(!bucket->empty());
      //      I(bucket->top()->getInst()->getAddr());

      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);

      // GMSG(getID()==1,"instqueue insert %p", bucket);
    } else {
      noFetch2.inc(getStatsFlag);
    }
  } else {
    noFetch.inc(getStatsFlag);
  }

  // RENAME Stage
  if(replayRecovering) {
    if((rROB.empty() && ROB.empty())) {
      // Recovering done
      EmulInterface *eint = TaskHandler::getEmul(flushing_fid);
      eint->syncHeadTail(flushing_fid);

      I(flushing);
      replayRecovering = false;
      flushing         = false;

      if((lastReplay + 2 * forwardProg_threshold) < replayID) {
        serialize_level = 3; // One over max to start with 2
        // MSG("%d Reset Serialize level @%lld : %lld %lld",cpu_id, globalClock,lastReplay, replayID);
      }
      if((lastReplay + forwardProg_threshold) > replayID) {
        if(serialize_level) {
          // MSG("%d One level less %d for %d @%lld : %lld %lld", cpu_id, serialize_level, serialize_for, globalClock, lastReplay,
          // replayID);
          serialize_level--;
        } else {
          // MSG("%d already at level 0 @%lld", cpu_id, globalClock);
        }
        serialize_for = serialize;
        // forwardProg_threshold = replayID - lastReplay;
        // serialize_for = forwardProg_threshold;
      }

      lastReplay = replayID;
    } else {
      nStall[ReplaysStall]->add(RealisticWidth, getStatsFlag);
      retire();
      return true;
    }
  }

  if(!pipeQ.instQueue.empty()) {
    spaceInInstQueue += issue(pipeQ);
  } else if(ROB.empty() && rROB.empty()) {
    // Still busy if we have some in-flight requests
    busy = pipeQ.pipeLine.hasOutstandingItems();
    return true;
  }

  retire();

  return true;
}
/* }}} */

void OoOProcessor::executing(DInst *dinst)
// {{{1 Called when the instruction starts to execute
{
  dinst->markExecuting();

#ifdef LATE_ALLOC_REGISTER
  if(dinst->getInst()->hasDstRegister())
    nTotalRegs--;
#endif
#ifdef TRACK_FORWARDING
  if(dinst->getStatsFlag()) {
    const Instruction *inst = dinst->getInst();
    avgNumSrc.sample(inst->getnsrc(), true);

    int nForward = 0;
    int nNeeded  = 0;
    if(inst->hasSrc1Register()) {
      nNeeded++;
      Time_t t = fwdDone[inst->getSrc1()];
      if((t + 2) >= globalClock)
        nForward++;
    }
    if(inst->hasSrc2Register()) {
      nNeeded++;
      Time_t t = fwdDone[inst->getSrc2()];
      if((t + 2) >= globalClock)
        nForward++;
    }

    if(nNeeded == 0)
      fwd0done0.inc(true);
    else if(nNeeded == 1) {
      if(nForward)
        fwd1done1.inc(true);
      else
        fwd1done0.inc(true);
    } else {
      if(nForward == 2)
        fwd2done2.inc(true);
      else if(nForward == 1)
        fwd2done1.inc(true);
      else
        fwd2done0.inc(true);
    }
  }
#endif
}
// 1}}}
//
void OoOProcessor::executed(DInst *dinst) {
#ifdef TRACK_FORWARDING
  fwdDone[dinst->getInst()->getDst1()] = globalClock;
  fwdDone[dinst->getInst()->getDst2()] = globalClock;
#endif
}

StallCause OoOProcessor::addInst(DInst *dinst)
/* rename (or addInst) a new instruction {{{1 */
{
  if(replayRecovering && dinst->getID() > replayID) {
    return ReplaysStall;
  }

  if((ROB.size() + rROB.size()) >= (MaxROBSize-1))
    return SmallROBStall;

  const Instruction *inst = dinst->getInst();

  if(nTotalRegs <= 0)
    return SmallREGStall;

  Cluster *cluster = dinst->getCluster();
  if(!cluster) {
    Resource *res = clusterManager.getResource(dinst);
    cluster       = res->getCluster();
    dinst->setCluster(cluster, res);
  }

  StallCause sc = cluster->canIssue(dinst);
  if(sc != NoStall) {
    return sc;
  }

  // if no stalls were detected do the following:
  //
  // BEGIN INSERTION (note that cluster already inserted in the window)
  // dinst->dump("");

#ifndef LATE_ALLOC_REGISTER
  if(inst->hasDstRegister()) {
    nTotalRegs--;
  }
#endif

  //#if 1
  if(!scooreMemory) { // no dynamic serialization for tradcore
    if(serialize_for > 0 && !replayRecovering) {
      serialize_for--;
      if(inst->isMemory() && dinst->isSrc3Ready()) {
        if(last_serialized && !last_serialized->isExecuted()) {
          // last_serialized->addSrc3(dinst); FIXME
          // MSG("addDep3 %8ld->%8lld %lld",last_serialized->getID(), dinst->getID(), globalClock);
        }
        last_serialized = dinst;
      }
    }
    //#else
  } else {
    if(serialize_for > 0 && !replayRecovering) {
      serialize_for--;

      if(serialize_level == 0) {
        // Serialize all the memory operations
        if(inst->isMemory() && dinst->isSrc3Ready()) {
          if(last_serialized && !last_serialized->isIssued()) {
            last_serialized->addSrc3(dinst);
          }
          last_serialized = dinst;
        }
      } else if(serialize_level == 1) {
        // Serialize stores, and loads depend on stores (no loads on loads)
        if(inst->isLoad() && dinst->isSrc3Ready()) {
          if(last_serializedST && !last_serializedST->isIssued()) {
            last_serializedST->addSrc3(dinst);
          }
          last_serialized = dinst;
        }
        if(inst->isStore() && dinst->isSrc3Ready()) {
          if(last_serialized && !last_serialized->isIssued()) {
            last_serialized->addSrc3(dinst);
          }
          last_serializedST = dinst;
        }
      } else {
        // Serialize if same register is being accessed
        if(inst->getSrc1() < LREG_ARCH0) {
          last_serializeLogical = inst->getSrc1();
        } else if(last_serializePC != dinst->getPC()) {
          last_serializeLogical = LREG_InvalidOutput;
        }
        last_serializePC = dinst->getPC();

        if(last_serializeLogical < LREG_ARCH0) {
          if(inst->isMemory()) {
            if(serializeRAT[last_serializeLogical]) {
              if(inst->isLoad()) {
                if(serializeRAT[last_serializeLogical]->getInst()->isStore())
                  serializeRAT[last_serializeLogical]->addSrc3(dinst);
              } else {
                serializeRAT[last_serializeLogical]->addSrc3(dinst);
              }
            }

            dinst->setSerializeEntry(&serializeRAT[last_serializeLogical]);
            serializeRAT[last_serializeLogical] = dinst;
          } else {
            serializeRAT[inst->getDst1()] = 0;
            serializeRAT[inst->getDst2()] = 0;
          }
        }
      }
    }
  }
  //#endif

  nInst[inst->getOpcode()]->inc(dinst->getStatsFlag()); // FIXME: move to cluster

  ROB.push(dinst);
  if(dinst->getInst()->isLoad()){
    if(loadIsSpec()){
      dinst->isSpec = true;
    } else {
      dinst->isSafe = true;
    }
  }

  I(dinst->getCluster() != 0); // Resource::schedule must set the resource field

  int n = 0;
  if(!dinst->isSrc2Ready()) {
    // It already has a src2 dep. It means that it is solved at
    // retirement (Memory consistency. coherence issues)
    if(RAT[inst->getSrc1()]) {
      RAT[inst->getSrc1()]->addSrc1(dinst);
      n++;
      // MSG("addDep0 %8ld->%8lld %lld",RAT[inst->getSrc1()]->getID(), dinst->getID(), globalClock);
    }
  } else {
    if(RAT[inst->getSrc1()]) {
      RAT[inst->getSrc1()]->addSrc1(dinst);
      n++;
      // MSG("addDep1 %8ld->%8lld %lld",RAT[inst->getSrc1()]->getID(), dinst->getID(), globalClock);
    }

    if(RAT[inst->getSrc2()]) {
      RAT[inst->getSrc2()]->addSrc2(dinst);
      n++;
      // MSG("addDep2 %8ld->%8lld %lld",RAT[inst->getSrc2()]->getID(), dinst->getID(), globalClock);
    }
  }
#ifdef TRACK_FORWARDING
  avgNumSrc.sample(inst->getnsrc(), dinst->getStatsFlag());
  avgNumDep.sample(n, dinst->getStatsFlag());
#endif

  dinst->setRAT1Entry(&RAT[inst->getDst1()]);
  dinst->setRAT2Entry(&RAT[inst->getDst2()]);

  I(!dinst->isExecuted());

  dinst->getCluster()->addInst(dinst);

  if(!dinst->isExecuted()) {
    RAT[inst->getDst1()] = dinst;
    RAT[inst->getDst2()] = dinst;
  }

  I(dinst->getCluster());

  dinst->markRenamed();

#ifdef WAVESNAP_EN
//add instruction to wavesnap
if(!SINGLE_WINDOW) {
  if(WITH_SAMPLING) {
    if(dinst->getStatsFlag()) {
      snap->add_instruction(dinst);
    }
  } else {
    snap->add_instruction(dinst);
  }
}
#endif

  return NoStall;
}
/* }}} */

void OoOProcessor::retire_lock_check()
/* Detect simulator locks and flush the pipeline {{{1 */
{
  RetireState state;
  if(active) {
    state.committed = nCommitted.getDouble();
  } else {
    state.committed = 0;
  }
  if(!rROB.empty()) {
    state.r_dinst    = rROB.top();
    state.r_dinst_ID = rROB.top()->getID();
  }

  if(!ROB.empty()) {
    state.dinst    = ROB.top();
    state.dinst_ID = ROB.top()->getID();
  }

  if(last_state == state && active) {
    I(0);
    MSG("Lock detected in P(%d), flushing pipeline", getID());
  }

  last_state = state;

  retire_lock_checkCB.scheduleAbs(globalClock + 100000);
}
/* }}} */

#ifdef ENABLE_LDBP
#if 0
void OoOProcessor::hit_on_load_table(DInst *dinst, bool is_li) {
  //if hit on load_table, update and move entry to LRU position
  for(int i = LOAD_TABLE_SIZE - 1; i >= 0; i--) {
    if(dinst->getPC() == load_table_vec[i].ldpc) {
      if(is_li) {
        load_table_vec[i].lt_load_imm(dinst);
      }else {
        load_table_vec[i].lt_load_hit(dinst);
      }
      load_table l = load_table_vec[i];
      load_table_vec.erase(load_table_vec.begin() + i);
      load_table_vec.push_back(load_table());
      load_table_vec[LOAD_TABLE_SIZE - 1] = l;
      if(load_table_vec[LOAD_TABLE_SIZE - 1].tracking > 0) {
        //append ld_ptr to PLQ
        int plq_idx = return_plq_index(load_table_vec[LOAD_TABLE_SIZE - 1].ldpc);
        if(plq_idx != - 1) {
          pending_load_queue p = plq_vec[plq_idx];
          plq_vec.erase(plq_vec.begin() + plq_idx);
        }else {
          plq_vec.erase(plq_vec.begin());
        }
        plq_idx = PLQ_SIZE - 1;
        plq_vec.push_back(pending_load_queue());
        plq_vec[plq_idx].load_pointer = load_table_vec[LOAD_TABLE_SIZE - 1].ldpc;
        if(load_table_vec[LOAD_TABLE_SIZE - 1].delta == load_table_vec[LOAD_TABLE_SIZE - 1].prev_delta) {
          plq_vec[plq_idx].plq_update_tracking(true);
        }else {
          plq_vec[plq_idx].plq_update_tracking(false);
        }
      }
      return;
    }
  }
  //if load_table miss
  load_table_vec.erase(load_table_vec.begin());
  load_table_vec.push_back(load_table());
  if(is_li) {
    load_table_vec[LOAD_TABLE_SIZE - 1].lt_load_imm(dinst);
  }else {
    load_table_vec[LOAD_TABLE_SIZE - 1].lt_load_miss(dinst);
  }
}

int OoOProcessor::return_load_table_index(AddrType pc) {
  //if hit on load_table, update and move entry to LRU position
  for(int i = LOAD_TABLE_SIZE - 1; i >= 0; i--) {
    if(pc == load_table_vec[i].ldpc) {
      return i;
    }
  }
  return -1;
}
#endif

void OoOProcessor::power_save_mode_table_reset() {
  //reset back end tables -> BTT, PLQ, CSB
  //reset front end tables -> BOT, LOR, LOT, CST

  //reset PLQ tracking to 0
  for(int i = 0; i < DL1->plq_vec.size(); i++) {
    DL1->plq_vec[i].tracking = 0;
  }

  //reset LOT and LOT
  for(int i = 0; i < DL1->lor_vec.size(); i++) {
    DL1->lor_vec[i].reset_entry();
    DL1->lot_vec[i].reset_valid();
  }

  //reset BTT
  std::vector<branch_trigger_table> btt;
  for(int x = 0; x < btt_vec.size(); x++) {
    btt.push_back(branch_trigger_table());
  }
  btt_vec.clear();
  btt_vec = btt;

  //reset BOT
  std::vector<MemObj::branch_outcome_table> bot;
  for(int j = 0; j < DL1->getBotSize(); j++) {
    bot.push_back(MemObj::branch_outcome_table());
  }
  DL1->bot_vec.clear();
  DL1->bot_vec = bot;
}

void OoOProcessor::rtt_load_hit(DInst *dinst) {
  //reset entry on hit and update fields
  RegType dst          = dinst->getInst()->getDst1();
  RegType src1          = dinst->getInst()->getSrc1();
  RegType src2          = dinst->getInst()->getSrc2();
  if(dst > LREG_R31) {
    //return;
  }
  int lt_idx = DL1->return_load_table_index(dinst->getPC());

  if((src1 == LREG_R0) && (src2 == LREG_R0)) { //if Li
    //rtt_vec[dst] = rename_tracking_table();
    rtt_vec[dst].reset_rtt();
    rtt_vec[dst].num_ops = 0;
    //rtt_vec[dst].load_table_pointer.clear();
    rtt_vec[dst].is_li = 1;
    rtt_vec[dst].pc_list.push_back(dinst->getPC());
    return;
    //no need to add anything to RTT's load pointer queue
  }

  if(lt_idx == -1) { //if load not in load table, no point checking RTT
    //reset RTT entry and exit
    //this is done so that, Br does not use LD data from an older LD with same dst reg
    rtt_vec[dst].reset_rtt();
    return;
  }


  if(DL1->load_table_vec[lt_idx].conf > DL1->getLoadTableConf()) {
    //rtt_vec[dst] = rename_tracking_table();
    rtt_vec[dst].reset_rtt();
    rtt_vec[dst].pc_list.clear();
    rtt_vec[dst].num_ops = 0;
    if(!DL1->load_table_vec[lt_idx].is_li) {
      //if not Li, the populate rtt
      rtt_vec[dst].load_table_pointer.push_back(dinst->getPC());
      rtt_vec[dst].pc_list.push_back(dinst->getPC());

      if(DL1->load_table_vec[lt_idx].chain_child) {
        DL1->load_table_vec[lt_idx].chain_set = false;
        DL1->load_table_vec[lt_idx].chain_child = false;
        DL1->load_table_vec[lt_idx].clear_chain_parent_ptr();
      }
    }
  }else {
    //the LD is not strided
    //check if it is a child in ld-ld chain
    bool all_ld_useful = true;
    if(1 || ((rtt_vec[src1].num_ops < NUM_OPS) && (rtt_vec[src1].load_table_pointer.size() < NUM_LOADS))) {
      for(int x = 0; x < rtt_vec[src1].load_table_pointer.size(); x++) {
        AddrType parent_pc = rtt_vec[src1].load_table_pointer[x];
        int parent_idx = DL1->return_load_table_index(rtt_vec[src1].load_table_pointer[x]);

        if((parent_idx == -1) || (DL1->load_table_vec[parent_idx].conf < DL1->getLoadTableConf())) {
          //if parent LD not in load table, then reset rtt entry(of child ld) and exit
          rtt_vec[dst].num_ops = NUM_OPS + 1;
          all_ld_useful = false;
          DL1->load_table_vec[lt_idx].chain_child = false;
          DL1->load_table_vec[lt_idx].clear_chain_parent_ptr();
          return;
        }

        int64_t parent_delta = DL1->load_table_vec[parent_idx].delta;
        DataType parent_data_delta = DL1->load_table_vec[parent_idx].data_delta;
        DataType parent_data = DL1->load_table_vec[parent_idx].ld_data;

        if(parent_delta == 0 || parent_data_delta == 0) {
          //MSG("OTHAY parent=%llx data=%d child=%llx", parent_pc, parent_data, dinst->getPC());
        }
#ifdef PRINT_LDBP
        bool is_conf = (DL1->load_table_vec[parent_idx].conf > DL1->getLoadTableConf());
        //MSG("OTHAX chain_length=%d parent=%llx conf=%d child=%llx", rtt_vec[src1].load_table_pointer.size(), parent_pc, is_conf, dinst->getPC());
#endif
        if(all_ld_useful) {
          DL1->load_table_vec[parent_idx].chain_parent = true;
          DL1->load_table_vec[parent_idx].chain_child = false;
          DL1->load_table_vec[parent_idx].chain_child_ptr = dinst->getPC();

          DL1->load_table_vec[lt_idx].chain_parent = false;
          DL1->load_table_vec[lt_idx].chain_child = true;
          DL1->load_table_vec[lt_idx].chain_parent_ptr[x] = parent_pc;
        }

      }
      rtt_vec[dst] = rename_tracking_table();
      if(all_ld_useful) {
        //ld's parent is conf, so push ld into rtt ld ptr list
        rtt_vec[dst].num_ops = 0;
        //rtt_vec[dst].load_table_pointer.push_back(parent_pc);
        rtt_vec[dst].load_table_pointer.push_back(dinst->getPC());
        //rtt_vec[dst].pc_list.push_back(dinst->getPC());
      }
    }else {
      //if not LD-LD chain, then reset entry
      rtt_vec[dst] = rename_tracking_table();
      rtt_vec[dst].num_ops = NUM_OPS + 1;
      rtt_vec[dst].pc_list.clear();
      rtt_vec[dst].pc_list.push_back(dinst->getPC());
    }
    rtt_vec[dst] = rename_tracking_table();
    rtt_vec[dst].num_ops = NUM_OPS + 1;
    rtt_vec[dst].pc_list.clear();
    rtt_vec[dst].pc_list.push_back(dinst->getPC());

    //rtt_vec[dst].num_ops = NUM_OPS + 1;
  }
}

void OoOProcessor::rtt_alu_hit(DInst *dinst) {
  //update fields using information from RTT and LT
  //num_ops = src1(nops) + src2(nops) + 1
  //ld_ptr = ptr of src1 and src2
  RegType dst  = dinst->getInst()->getDst1();
  RegType src1 = dinst->getInst()->getSrc1();
  RegType src2 = dinst->getInst()->getSrc2();

  int nops1 = rtt_vec[src1].num_ops;
  int nops2 = rtt_vec[src2].num_ops;

  int pc_list_size1 = rtt_vec[src1].pc_list.size();
  int pc_list_size2 = rtt_vec[src2].pc_list.size();

  if(src1 == LREG_R0 && src2 == LREG_R0) { //if ALU is Li
    rtt_vec[dst].num_ops = 0 + 0 + 1;
    rtt_vec[dst].load_table_pointer.clear();
    return;
  }
  if(src1 == LREG_R0) {
    nops1 = 0;
  }
  if(src2 == LREG_R0) {
    nops2 = 0;
  }
  //concatenate ptr list of 2 srcs into dst
  std::vector<AddrType> tmp;
  tmp.clear();
  tmp.insert(tmp.begin(), rtt_vec[src1].load_table_pointer.begin(), rtt_vec[src1].load_table_pointer.end());
  tmp.insert(tmp.end(), rtt_vec[src2].load_table_pointer.begin(), rtt_vec[src2].load_table_pointer.end());
  //tmp.insert(tmp.begin(), rtt_vec[src1].load_table_pointer.begin(), rtt_vec[src1].load_table_pointer.begin() + rtt_vec[src1].load_table_pointer.size());
  //tmp.insert(tmp.end(), rtt_vec[src2].load_table_pointer.begin(), rtt_vec[src2].load_table_pointer.begin() + rtt_vec[src2].load_table_pointer.size());
  std::sort(tmp.begin(), tmp.end());
  tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());

  //concatenate pc list of 2 srcs into dst
  std::vector<AddrType> tmp2;
  tmp2.clear();
  tmp2.insert(tmp2.begin(), rtt_vec[src1].pc_list.begin(), rtt_vec[src1].pc_list.end());
  tmp2.insert(tmp2.end(), rtt_vec[src2].pc_list.begin(), rtt_vec[src2].pc_list.end());
  tmp2.push_back(dinst->getPC()); //push current ALU PC
  std::sort(tmp2.begin(), tmp2.end());
  tmp2.erase(std::unique(tmp2.begin(), tmp2.end()), tmp2.end());
#if 0
  //don't increment NOPS if ALU part of LD-BR slice with use_slice == 0
  for(int i = 0; i < rtt_vec[src1].load_table_pointer.size(); i++) {
    int lt_id = DL1->return_load_table_index(rtt_vec[src1].load_table_pointer[i]);
    if(lt_id != - 1 && (DL1->load_table_vec[lt_id].use_slice < 1)) {
      nops1 = 0;
      break;
    }
  }

  for(int i = 0; i < rtt_vec[src2].load_table_pointer.size(); i++) {
    int lt_id = DL1->return_load_table_index(rtt_vec[src2].load_table_pointer[i]);
    if(lt_id != - 1 && (DL1->load_table_vec[lt_id].use_slice < 1)) {
      nops2 = 0;
      break;
    }
  }
#endif

  int nops = nops1 + nops2 + 1;
  rtt_vec[dst].num_ops = nops;
  rtt_vec[dst].load_table_pointer = tmp;
  rtt_vec[dst].pc_list = tmp2;
}

void OoOProcessor::rtt_br_hit(DInst *dinst) {
  //read num ops and stride ptr fields on hit
  //use stride ptr to figure out if the LD(s) are predictable
  int btt_id = return_btt_index(dinst->getPC());
  if(btt_id == -1) { //do the below checks only if BTT_MISS
    if(dinst->isBiasBranch()) {
      //Index RTT with BR only when tage is not high-conf
      return;
    }else if(!dinst->isBranchMiss()) {
      return;
    }
  }
  num_bad_tage_br.inc(true); //stats for num branches which are hard to predict for tage


  RegType src1 = dinst->getInst()->getSrc1();
  RegType src2 = dinst->getInst()->getSrc2();

  bool all_ld_conf1 = true; //all lds+chain lds conf for br src1
  bool all_ld_conf2 = true; //all lds+chain lds conf for br src2
  int ld_conf1 = 1; //all non-chain LDs conf for br src1
  int ld_conf2 = 1; //all non-chain LDs conf for br src2
  int nops1 = rtt_vec[src1].num_ops;
  int nops2 = rtt_vec[src2].num_ops;
  bool is_chain1    = false; //flag -> Br src1 is from a ld-ld chain
  bool is_chain2    = false; //flag -> Br src2 is from a ld-ld chain
  bool predictable1 = false; //Br src1 LD not predictable
  bool predictable2 = false; //Br src2 LD not predictable
  int num_lds1          = rtt_vec[src1].load_table_pointer.size();
  int num_lds2          = rtt_vec[src2].load_table_pointer.size();
  int num_chain_parent1 = 0;
  int num_chain_parent2 = 0;
  bool no_src2 = false; // if Br does not have src2
  bool chain_ld_conf1 = false; //if chain parent LDs are conf for Br src1
  bool chain_ld_conf2 = false; //if chain parent LDs are conf for Br src2

  int pc_list_size1 = rtt_vec[src1].pc_list.size();
  int pc_list_size2 = rtt_vec[src2].pc_list.size();
#ifdef PRINT_LDBP
  //std::vector<AddrType> brpc_list{0x1044e}; //BDATA
  //std::vector<AddrType> brpc_list{0x11010, 0x11028, 0x11044, 0x112e2, 0x11048}; //BFS
  //std::vector<AddrType> brpc_list{0x119da, 0x119c6}; //PR
  //std::vector<AddrType> brpc_list{0x128b8, 0x12886}; //BC
  std::vector<AddrType> brpc_list{0x10ada, 0x10b48, 0x10b12, 0x10c72, 0x10b2e}; //CC
  //std::vector<AddrType> brpc_list{0x11f94, 0x11f68, 0x11f32}; //SSSP
  //std::vector<AddrType> brpc_list{0x11d12, 0x11d20, 0x164ea, 0x164dc, 0x11d24, 0x11630}; //TC
  //std::vector<AddrType> brpc_list{0x1e8fe, 0x17926, 0x218fc, 0x218fe, 0x1e908, 0x1917c, 0x1e00a, 0x1e014, 0x1d8de}; //SJENG
  //std::vector<AddrType> brpc_list{0x12528, 0x13042, 0x13020, 0x13066, 0x13044, 0x13022, 0x123c8, 0x13622, 0x12ffc, 0x1386c}; //ASTAR
  //std::vector<AddrType> brpc_list{0x122be, 0x12328, 0x12388, 0x15806, 0x1199a, 0x1285c, 0x127d4, 0x154da}; //BZIP2
  //std::vector<AddrType> brpc_list{0x10f42, 0x10f2c, 0x10eac, 0x10f5c}; //MCF
  //std::vector<AddrType> brpc_list{0x7de42, 0xa7b6e, 0x7edb8, 0x7de88, 0x30d5e, 0xa4f94, 0xa4f9c}; //GOBMK
  //std::vector<AddrType> brpc_list{0x18736, 0x186a4, 0x187b2, 0x186fa, 0x18686}; //HMMER

  //print LD-BR chains
  //f(std::find(brpc_list.begin(), brpc_list.end(), dinst->getPC()) != brpc_list.end()) {}

  if((pc_list_size1 < (NUM_OPS + NUM_LOADS)) && (pc_list_size2 < (NUM_OPS + NUM_LOADS))) {
    MSG("LDBR,clk=%d,br=%llx,outcome=%s,ld1=%d,ld2=%d", globalClock, dinst->getPC(), dinst->isTaken()?"T":"NT", num_lds1, num_lds2);
    for(int i = 0; i < pc_list_size1; i++) {
      MSG("pc1=%llx", rtt_vec[src1].pc_list[i]);
    }
    for(int j = 0; j < pc_list_size2; j++) {
      MSG("pc2=%llx", rtt_vec[src2].pc_list[j]);
    }
  }
#endif


#if 1
  //for loop checks if Br's associated LDs are conf
  for(int i = 0; i < rtt_vec[src1].load_table_pointer.size(); i++) {
    AddrType ld_pc = rtt_vec[src1].load_table_pointer[i];
    int lt_index = DL1->return_load_table_index(ld_pc);
    //Above line can cause SEG_FAULT if index of Li is checked
    if(lt_index != -1) {
      if((DL1->load_table_vec[lt_index].conf > DL1->getLoadTableConf())) {
        all_ld_conf1 = true;
      }else {
        //if ld not conf
        // check if its a chain child
        all_ld_conf1 = false;
        ld_conf1 = 0;
        break;
      }
    }else {
      all_ld_conf1 = false;
      break;
    }
  }

  //for loop checks if Br's associated LDs are conf
  for(int i = 0; i < rtt_vec[src2].load_table_pointer.size(); i++) {
    AddrType ld_pc = rtt_vec[src2].load_table_pointer[i];
    int lt_index = DL1->return_load_table_index(ld_pc);
    //Above line can cause SEG_FAULT if index of Li is checked
    if(lt_index != -1) {
      if((DL1->load_table_vec[lt_index].conf > DL1->getLoadTableConf())) {
        all_ld_conf2 = true;
      }else {
        //if ld not conf
          //if not chain child, then LD not predictable
        all_ld_conf2 = false;
        ld_conf2 = 0;
        break;
      }
    }else {
      all_ld_conf2 = false;
      break;
    }
  }
#endif

  if(src2 == LREG_R0) {
    //all_ld_conf2 = false;
    nops2 = 0;
    no_src2 = true;
  }
  int nops     = nops1 + nops2;
  int nlds     = rtt_vec[src1].load_table_pointer.size() + rtt_vec[src2].load_table_pointer.size();
  bool all_conf = all_ld_conf1 && all_ld_conf2;
  //bool is_chain = is_chain1 && is_chain2;

  std::vector<int> ld_flag = std::vector<int>(NUM_LOADS);
  std::vector<int> ld_flag_complex = std::vector<int>(NUM_LOADS);
  int ld_count = 0; //to index ld_flag[]
  // LDBR with complex ops or num_lds > NUM_LDS (0)
  // LDBR with LD i not delta or data conf or chain ld(1)
  // LD with constant stride flag (2)
  // LD with constant data flag (3)
  // ld-ld chain flag (4)
  // ldbr with qsort like swap flag (5)
  // LDBR with nops > NUM_OPS and nops < 8 (6)
  // LDBR with nops > 8 and nops < 16 (7)
  // LDBR with nops > 16  (8)
  for(int i = 0; i < NUM_LOADS; i++) {
    ld_flag[i] = 0;
    ld_flag_complex[i] = 0;
  }

#if 1
  bool op_condition = ((nops < NUM_OPS) && (nlds < NUM_LOADS));
  //bool op_condition = (nops < NUM_OPS);

  if(op_condition) { //if LDBR chain has nops < NUM_OPS, then classify type
    for(int i = 0; i < num_lds1; i++) {
      AddrType ld_pc = rtt_vec[src1].load_table_pointer[i];
      int lt_index = DL1->return_load_table_index(ld_pc);
      //Above line can cause SEG_FAULT if index of Li is checked
      if(lt_index != -1 && (ld_count < NUM_LOADS)) {
        if((DL1->load_table_vec[lt_index].data_conf > DL1->getLoadDataConf())) {
          ld_flag[ld_count] = 3;
          ld_count++;
          continue;
        }else {
          ld_flag[ld_count] = 1;
        }

        if((DL1->load_table_vec[lt_index].conf > DL1->getLoadTableConf())) {
          ld_flag[ld_count] = 2;
          ld_count++;
          continue;
        }else {
          ld_flag[ld_count] = 1;
        }

        if(DL1->load_table_vec[lt_index].chain_child) {
          ld_flag[ld_count] = 4;
          ld_count++;
          continue;
        }else {
          ld_flag[ld_count] = 1;
        }
        //FIXME: Add Qsort flag
        ld_count++;
      }
    }

    for(int i = 0; i < num_lds2; i++) {
      AddrType ld_pc = rtt_vec[src2].load_table_pointer[i];
      int lt_index = DL1->return_load_table_index(ld_pc);
      if(lt_index != -1 && (ld_count < NUM_LOADS)) {
        if((DL1->load_table_vec[lt_index].data_conf > DL1->getLoadDataConf())) {
          ld_flag[ld_count] = 3;
          ld_count++;
          continue;
        }else {
          ld_flag[ld_count] = 1;
        }
        if((DL1->load_table_vec[lt_index].conf > DL1->getLoadTableConf())) {
          ld_flag[ld_count] = 2;
          ld_count++;
          continue;
        }else {
          ld_flag[ld_count] = 1;
        }
        if(DL1->load_table_vec[lt_index].chain_child) {
          ld_flag[ld_count] = 4;
          ld_count++;
          continue;
        }else {
          ld_flag[ld_count] = 1;
        }
        ld_count++;
      }
    }

  }else if(0 && (rtt_vec[src1].is_complex || rtt_vec[src2].is_complex)){
    //if src1 or src2 is complex
    for(int i = 0; i < ld_flag.size(); i++) {
      ld_flag[i] = 0;
    }
  }
#endif


#if 0
  if(dinst->isBranchMiss_level2() && (nops < NUM_LOADS && nlds < NUM_LOADS))
    MSG("RTT brpc=%llx nops=%d nloads=%d all_conf=%d", dinst->getPC(), nops, nlds, all_conf);
#endif

  /*
   *************************
   *COLLECT LDBR CHAIN STATS
   *************************
   * */
  if(src2 == LREG_R0) {
    num_br_1_src.inc(true);
  }else {
    num_br_2_src.inc(true);
  }


  bool is_chain = false;
  for(int i = 0; i < ld_flag.size(); i++) {
    if(ld_flag[i] == 4) {
      is_chain      = true;
    }
  }

  bool is_complex = rtt_vec[src1].is_complex || rtt_vec[src2].is_complex;
  bool ldbp_condition = all_conf && (nops < NUM_OPS) && (nlds < NUM_LOADS);
  bool ld_cond = (nlds < NUM_LOADS);
  bool op_cond = (nops < NUM_OPS);
  num_ldbr.inc(ldbp_condition);
  num_non_ldbr.inc(!ldbp_condition);
  if(is_chain) {
    num_br_chain.inc(true);
    num_br_chain_ldbp.inc(all_conf && (nlds < NUM_LOADS) && op_cond);
    num_br_chain_non_ldbp.inc(!all_conf);
    num_br_chain_x_ld.inc(all_conf && !ld_cond && op_cond);
    num_br_chain_x_op.inc(all_conf && ld_cond && !op_cond);
  }else if(nops == 0) {
    num_br_trivial.inc(true);
    num_br_trivial_ldbp.inc(all_conf && (nlds < NUM_LOADS));
    num_br_trivial_non_ldbp.inc(!all_conf);
    num_br_trivial_x_ld.inc(all_conf && !ld_cond);
  }else if(nops > 0 && nops < NUM_OPS) {
    num_br_simple.inc(true);
    num_br_simple_ldbp.inc(all_conf && (nlds < NUM_LOADS));
    num_br_simple_non_ldbp.inc(!all_conf);
    num_br_simple_x_ld.inc(all_conf && !ld_cond);
  }else if(is_complex) {
    num_br_complex.inc(true);
  }else if(nops >= NUM_OPS) {
    num_br_excess.inc(true);
    num_br_excess_ldbp.inc(all_conf && (nlds < NUM_LOADS));
    num_br_excess_non_ldbp.inc(!all_conf);
    num_br_excess_x_ld.inc(all_conf && !ld_cond);
  }

  int64_t ld_flag_val = 0;
  for(int i = 0; i < ld_flag.size(); i++) {
    int64_t digit    = pow(10, i);
    ld_flag_val += (digit * ld_flag[i]);
  }
  //MSG("LDBR br=%llx ld_flag=%d:", dinst->getPC(), ld_flag_val);

  bool cond1 = ((nops >= NUM_OPS) && (nops < 32));
  bool cond2 = (nlds >= NUM_LOADS) && ((nops >= NUM_OPS) && (nops < 32));
  bool cond3 = (nlds < NUM_LOADS) && ((nops >= NUM_OPS) && (nops < 32));

  if(ld_flag_val == 0) {
    if(is_complex) {
      ld_flag_val = 0;
    }else if(cond3 && all_conf && !is_complex) {
      ld_flag_val = 623;
    }else if(cond3 && !all_conf && !is_complex) {
      ld_flag_val = 61;
    }else if(cond2 && all_conf && !is_complex) {
      ld_flag_val = 723;
    }else if(cond2 && !all_conf && !is_complex) {
      ld_flag_val = 71;
    }
  }

  //Update LDBR type counts
  if(ld_flag_val == 0 || rtt_vec[src1].is_complex || rtt_vec[src2].is_complex || (ld_flag_val == 6000000)|| (ld_flag_val == 7000000)|| (ld_flag_val == 8000000)) {
    num_ldbr_0.inc(true); //LDBR with complex ops or num_lds > NUM_LDS
  }else if(ld_flag_val == 2) {
    num_ldbr_2.inc(true);
  }else if(ld_flag_val == 3) {
    num_ldbr_3.inc(true);
  }else if(ld_flag_val == 4) {
    num_ldbr_4.inc(true);
  }else if(ld_flag_val == 12) {
    num_ldbr_12.inc(true);
  }else if(ld_flag_val == 22) {
    num_ldbr_22.inc(true);
  }else if(ld_flag_val == 23) {
    num_ldbr_23.inc(true);
  }else if(ld_flag_val == 24) {
    num_ldbr_24.inc(true);
  }else if(ld_flag_val == 32) {
    num_ldbr_32.inc(true);
  }else if(ld_flag_val == 33) {
    num_ldbr_33.inc(true);
  }else if(ld_flag_val == 34) {
    num_ldbr_34.inc(true);
  }else if(ld_flag_val == 44) {
    num_ldbr_44.inc(true);
  }else if(ld_flag_val == 222) {
    num_ldbr_222.inc(true);
  }else if(ld_flag_val == 223) {
    num_ldbr_223.inc(true);
  }else if(ld_flag_val == 233) {
    num_ldbr_233.inc(true);
  }else if(ld_flag_val == 423) {
    num_ldbr_423.inc(true);
  }else if(ld_flag_val == 444) {
    num_ldbr_444.inc(true);
  }else if(ld_flag_val == 2222) {
    num_ldbr_2222.inc(true);
  }else if(ld_flag_val == 2223) {
    num_ldbr_2223.inc(true);
  }else if(ld_flag_val == 2232) {
    num_ldbr_2232.inc(true);
  }else if(ld_flag_val == 2322) {
    num_ldbr_2322.inc(true);
  }else if(ld_flag_val == 2233) {
    num_ldbr_2233.inc(true);
  }else if(ld_flag_val == 623) {
    num_ldbr_6L2_3.inc(true);
  }else if(ld_flag_val == 61) {
    num_ldbr_6L1.inc(true);
  }else if(ld_flag_val == 723) {
    num_ldbr_7L2_3.inc(true);
  }else if(ld_flag_val == 71) {
    num_ldbr_7L1.inc(true);
  }else {
    num_ldbr_others.inc(true);
  }

  //reset nops counter in rtt_vec
  //rtt_vec[src1].num_ops = 0;
  //rtt_vec[src2].num_ops = 0;

  //if all_conf && nops < num_ops && nlds < num_loads
  //fill BTT with brpc and str ptrs; acc = 4; sp.tracking = true for all str ptrs

  int num_ops1     = 2;
  int num_ops2     = 2;
  int num_csb_src1 = num_lds1 + nops1;
  int num_csb_src2 = num_lds2 + nops2;

  if(all_conf && (nops < NUM_OPS) && (nlds < NUM_LOADS)) {
  //if(all_conf && (nops < NUM_OPS) && (nlds < NUM_LOADS) && (num_csb_src1 < num_ops1) && (num_csb_src2 < num_ops2)) {}
    //if BTT miss and TAGE low conf
    if(btt_id == -1) {
      btt_br_miss(dinst);
    }else {
      btt_br_hit(dinst, btt_id);
    }
  }else {
    /***************
     *************
      get LD stats for LD-BR chains which cannot be fixed by LDBP
    *************
    ***************/
    for(int i = 0; i < rtt_vec[src1].load_table_pointer.size(); i++) {
      AddrType ld_pc = rtt_vec[src1].load_table_pointer[i];
      int lt_idx = DL1->return_load_table_index(ld_pc);
      num_loads_non_ldbp.inc(true); //inc load counter
      if((lt_idx != -1)) {
        if(DL1->load_table_vec[lt_idx].conf > DL1->getLoadTableConf())
          num_ld_conf_non_ldbp.inc(true);
        if(DL1->load_table_vec[lt_idx].data_conf > DL1->getLoadDataConf())
          num_ld_data_conf_non_ldbp.inc(true);
        if(DL1->load_table_vec[lt_idx].chain_child)
          num_chain_child.inc(true);
      }
    }

    for(int i = 0; i < rtt_vec[src2].load_table_pointer.size(); i++) {
      AddrType ld_pc = rtt_vec[src2].load_table_pointer[i];
      int lt_idx = DL1->return_load_table_index(ld_pc);
      num_loads_non_ldbp.inc(true); //inc load counter
      if((lt_idx != -1)) {
        if(DL1->load_table_vec[lt_idx].conf > DL1->getLoadTableConf())
          num_ld_conf_non_ldbp.inc(true);
        if(DL1->load_table_vec[lt_idx].data_conf > DL1->getLoadDataConf())
          num_ld_data_conf_non_ldbp.inc(true);
        if(DL1->load_table_vec[lt_idx].chain_child)
          num_chain_child.inc(true);
      }
    }
    /*********************************************
     * *****
     *
     *
     * *******************************************/

    //MSG("LDBR2 brpc=%llx lds=%d ops=%d", dinst->getPC(), nlds, nops);
    // something wrong with Br (either delta changed, or LD-BR slice changed)
    // remove BTT entry if it already exists for this Br pc
    // clear corresponding PLQ and LT tracking fields
    // clear BOT, LOR and LOT entries
    if(btt_id != -1) {
      int bot_id = DL1->return_bot_index(dinst->getPC());
      for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
        int load_table_id = DL1->return_load_table_index(btt_vec[btt_id].load_table_pointer[i]);
        int plq_id = DL1->return_plq_index(btt_vec[btt_id].load_table_pointer[i]);
        //int lor_id = DL1->return_lor_index(btt_vec[btt_id].load_table_pointer[i]);
        int lor_id = DL1->compute_lor_index(dinst->getPC(), btt_vec[btt_id].load_table_pointer[i]);

        if(load_table_id != -1) {
          DL1->load_table_vec[load_table_id].tracking = 0;
        }

        if(plq_id != -1) {
          DL1->plq_vec[plq_id].tracking = 0;
        }

        //clear LOR entry and corresponding LOT entry
        if(lor_id != -1) {
          DL1->lor_vec[lor_id].reset_entry();
          DL1->lot_vec[lor_id].reset_valid();
        }
#if 0
          //clear LOR entry
          DL1->lor_vec.erase(DL1->lor_vec.begin() + lor_id);
          DL1->lor_vec.push_back(MemObj::load_outcome_reg());
          //clear corresponding LOT entry
          //DL1->lot_vec[lor_id].reset_valid();
          DL1->lot_vec.erase(DL1->lot_vec.begin() + lor_id);
          DL1->lot_vec.push_back(MemObj::load_outcome_table());
#endif

      }
      //clear BOT entry
      if(bot_id != -1) {
#if 0
        DL1->bot_vec.erase(DL1->bot_vec.begin() + bot_id);
        DL1->bot_vec.push_back(MemObj::branch_outcome_table());
#endif

#if 1
        std::vector<MemObj::branch_outcome_table> bot;
        for(int j = 0; j < DL1->getBotSize(); j++) {
          if(j != bot_id) {
            bot.push_back(DL1->bot_vec[j]);
          }
        }
        DL1->bot_vec.clear();
        DL1->bot_vec = bot;
        DL1->bot_vec.push_back(MemObj::branch_outcome_table());
#endif
      }
      //clear BTT entry
#if 0
      btt_vec.erase(btt_vec.begin() + btt_id);
      btt_vec.push_back(branch_trigger_table());
#endif
      std::vector<branch_trigger_table> btt;
      for(int x = 0; x < btt_vec.size(); x++) {
        if(x != btt_id) {
          btt.push_back(btt_vec[x]);
        }
      }
      btt_vec.clear();
      btt_vec = btt;
      btt_vec.push_back(branch_trigger_table());
    }
  }
}

int OoOProcessor::return_btt_index(AddrType pc) {
  //if hit on BTT, update and move entry to LRU position
  for(int i = 0; i < BTT_SIZE; i++) {
    if(pc == btt_vec[i].brpc) {
      return i;
    }
  }
  return -1;
}

void OoOProcessor::btt_br_miss(DInst *dinst) {
  RegType src1 = dinst->getInst()->getSrc1();
  RegType src2 = dinst->getInst()->getSrc2();
#if 0
  btt_vec.erase(btt_vec.begin());
  btt_vec.push_back(branch_trigger_table());
#endif
  //remove BTT element at id=0 and push new element to LRU position
  std::vector<branch_trigger_table> btt;
  for(int x = 1; x < btt_vec.size(); x++) {
    btt.push_back(btt_vec[x]);
  }
  btt_vec.clear();
  btt_vec = btt;
  btt_vec.push_back(branch_trigger_table());
  //merge LD ptr list from src1 and src2 entries in RTT
  std::vector<AddrType> tmp;
  std::vector<AddrType> tmp_addr;
  tmp.clear();
  tmp.insert(tmp.begin(), rtt_vec[src1].load_table_pointer.begin(), rtt_vec[src1].load_table_pointer.end());
  tmp.insert(tmp.end(), rtt_vec[src2].load_table_pointer.begin(), rtt_vec[src2].load_table_pointer.end());
  //tmp.insert(tmp.begin(), rtt_vec[src1].load_table_pointer.begin(), rtt_vec[src1].load_table_pointer.begin() + rtt_vec[src1].load_table_pointer.size());
  //tmp.insert(tmp.end(), rtt_vec[src2].load_table_pointer.begin(), rtt_vec[src2].load_table_pointer.begin() + rtt_vec[src2].load_table_pointer.size());
  std::sort(tmp.begin(), tmp.end());
  tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());
  btt_vec[BTT_SIZE - 1].load_table_pointer = tmp;
  btt_vec[BTT_SIZE - 1].brpc = dinst->getPC();
  btt_vec[BTT_SIZE - 1].accuracy = BTT_MAX_ACCURACY / 2; //bpred accuracy set to intermediate value
  for(int i = 0; i < tmp.size(); i++) { //update sp.tracking for all ld_ptr in BTT
    int id = DL1->return_load_table_index(tmp[i]);
    if(id != -1) {
      DL1->load_table_vec[id].lt_update_tracking(true);
      //allocate entries at LOR and LOT
      // set LOR_START to next LD-BR chain's ld_addr
      // When current BR retires, there maybe other BRs inflight
      // set ld_start considering these inflight BRs
      //AddrType lor_start_addr = load_table_vec[id].ld_addr + (load_table_vec[id].delta * TRIG_LD_JUMP);
      //int look_ahead = dinst->getInflight() + 1;
      int look_ahead = 0;
      //lor.start -> lor.start + lor.delta
      //(as this allocate is done on Br miss, there will be no inflight Br. So lor.start should be updated to next LD-BR's addr)
      AddrType lor_start_addr = DL1->load_table_vec[id].ld_addr + DL1->load_table_vec[id].delta;
      //tmp_addr.push_back(DL1->load_table_vec[id].ld_addr);
      tmp_addr.push_back(lor_start_addr);
      bool is_li     = false;
      if(DL1->load_table_vec[id].ld_addr == 0) {
        lor_start_addr = 0;
        is_li          = true;
      }
      DL1->lor_allocate(dinst->getPC(), DL1->load_table_vec[id].ldpc, lor_start_addr, DL1->load_table_vec[id].delta, look_ahead, is_li);
      DL1->bot_allocate(dinst->getPC(), DL1->load_table_vec[id].ldpc, DL1->load_table_vec[id].ld_addr);
#if 0
      if(dinst->getPC() == 0x1044e)
        MSG("BOT_ALLOC clk=%d brid=%d brpc=%llx ldpc=%llx ld_addr=%d lor_start=%d", globalClock, dinst->getID(), dinst->getPC(), tmp[i], DL1->load_table_vec[id].ld_addr, lor_start_addr);
#endif
    }
  }
  int bot_id                    = DL1->return_bot_index(dinst->getPC());
  if(bot_id != -1) {
    DL1->bot_vec[bot_id].load_ptr = tmp;
    DL1->bot_vec[bot_id].curr_br_addr = tmp_addr;
  }
}

void OoOProcessor::btt_br_hit(DInst *dinst, int btt_id) {
  btt_vec[btt_id].btt_update_accuracy(dinst, btt_id); //update BTT accuracy
  if(btt_vec[btt_id].accuracy == 0) { //clear sp.tracking if acc == 0 and reset BTT entry
    for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
      int lt_idx = DL1->return_load_table_index(btt_vec[btt_id].load_table_pointer[i]);
      if(lt_idx != -1) {
        DL1->load_table_vec[lt_idx].tracking = 0;
      }
    }
    //clear entry
    btt_vec.erase(btt_vec.begin() + btt_id);
    btt_vec.push_back(branch_trigger_table());
    return;
  }
  //check if btt_strptr == plq_strptr and tracking bits are set
  //if yes and associated LOR are tracking the str_ptr, then trigger loads

  int tl_flag = btt_pointer_check(dinst, btt_id);
  //int tl_flag = 1;
  if(tl_flag == 1) { //trigger loads
    //trigger loads
    for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
      if(dinst->getPC() == 0x10eac) {
        MSG("LOAD_PTR_%d br=%llx ld=%llx", i, dinst->getPC(), btt_vec[btt_id].load_table_pointer[i]);
      }
      btt_trigger_load(dinst, btt_vec[btt_id].load_table_pointer[i]);
    }
  }
}

void OoOProcessor::btt_trigger_load(DInst *dinst, AddrType ld_ptr) {
  //int lor_id = DL1->return_lor_index(ld_ptr);
  int lor_id = DL1->compute_lor_index(dinst->getPC(), ld_ptr);
  int lt_idx = DL1->return_load_table_index(ld_ptr);
  if(lt_idx == -1) {
    return;
  }
  //int use_slice = DL1->load_table_vec[lt_idx].use_slice;
  int use_slice = 1;
  if(lor_id != -1 && (DL1->lor_vec[lor_id].brpc == dinst->getPC()) && (DL1->lor_vec[lor_id].ld_pointer == ld_ptr)) {
    AddrType lor_start_addr = DL1->lor_vec[lor_id].ld_start;
    AddrType lt_load_addr = DL1->load_table_vec[lt_idx].ld_addr;
    AddrType trig_ld_dist = DL1->lor_vec[lor_id].trig_ld_dist;
    int64_t lor_delta = DL1->lor_vec[lor_id].ld_delta;
    int num_trig_ld = 0;
    //MSG("TL brpc=%llx ldpc=%llx ld_addr=%d conf=%d", dinst->getPC(), ld_ptr, lor_start_addr, DL1->load_table_vec[lt_idx].conf);
    if(use_slice > 0) {
      if(trig_ld_dist <= (MAX_TRIG_DIST/2)) {
        num_trig_ld = 4;
      }else if(trig_ld_dist > (MAX_TRIG_DIST/2) && trig_ld_dist < MAX_TRIG_DIST) {
        num_trig_ld = 2;
      }else {
        num_trig_ld  = 1;
        trig_ld_dist = MAX_TRIG_DIST;
      }

      for(int i = 0; i < num_trig_ld; i++) {
        //AddrType trigger_addr = lor_start_addr + (lor_delta * (31 + i)); //trigger few delta ahead of current ld_addr
        AddrType trigger_addr = lor_start_addr + (lor_delta * (trig_ld_dist + i)); //trigger few delta ahead of current ld_addr
#if 0
        if(dinst->getPC() == 0x119da || dinst->getPC() == 0x119c6)
        MSG("TL clk=%d br_id=%d brpc=%llx ldpc=%llx ld_addr=%d curr_lor_start=%d trig_addr=%d del=%d lor_id=%d", globalClock, dinst->getID(), dinst->getPC(), ld_ptr, lt_load_addr, lor_start_addr, trigger_addr, lor_delta, lor_id);
#endif
        MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ld_ptr, dinst->getPC(), lor_start_addr, 0, lor_delta, inflight_branch, 0, 0, 0, 0);
        //if(trig_ld_dist < 64)
        if(trig_ld_dist < MAX_TRIG_DIST)
          DL1->lor_vec[lor_id].trig_ld_dist++;
      }
      //update lor_start by delta so that next TL doesnt trigger redundant loads
      DL1->lor_vec[lor_id].ld_start = DL1->lor_vec[lor_id].ld_start + DL1->lor_vec[lor_id].ld_delta;
      DL1->lor_vec[lor_id].data_pos++;
      if(DL1->lor_vec[lor_id].data_pos > (DL1->getLotQueueSize() - 1)) {
        DL1->lor_vec[lor_id].data_pos = DL1->lor_vec[lor_id].data_pos % DL1->getLotQueueSize();
      }
      //DL1->lor_vec[lor_id].data_pos++;
      //set LT.use_slice to 0; ensures that next time TL is triggered only when LD is needed by BR
      DL1->load_table_vec[lt_idx].use_slice = 0;
      DL1->lor_vec[lor_id].use_slice = 1;
    }else {
      //use_slice == 0
      DL1->lor_vec[lor_id].use_slice = 0;
#if 1
      int bot_id = DL1->return_bot_index(dinst->getPC());
      if(bot_id != -1) {
        int lot_qidx = DL1->bot_vec[bot_id].outcome_ptr % DL1->getLotQueueSize();
        DL1->lot_vec[lor_id].valid[lot_qidx] = 1;
        //find br-flip -> no updates happen when br flips
        //br_flip is !curr_br_outcome
        //-> coz use_slice == 0 when curr_br_outcome, so the use_slice = 1 when Br has !curr-br-outcome
        DL1->bot_vec[bot_id].br_flip = !dinst->isTaken();
      }
#endif
    }
  }
}

int OoOProcessor::btt_pointer_check(DInst *dinst, int btt_id) {
  //returns 1 if all ptrs are found; else 0
  bool all_ptr_found = true;
  bool all_ptr_track = true;
  std::vector<AddrType> tmp_plq;

#if 1
  for(int i = 0; i < DL1->plq_vec.size(); i++) {
    tmp_plq.push_back(DL1->plq_vec[i].load_pointer);
  }
#endif

  for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
    AddrType ldpc = btt_vec[btt_id].load_table_pointer[i];
    int lt_idx = DL1->return_load_table_index(btt_vec[btt_id].load_table_pointer[i]);
    if(!DL1->load_table_vec[lt_idx].chain_child) {
      auto it = std::find(tmp_plq.begin(), tmp_plq.end(), btt_vec[btt_id].load_table_pointer[i]);
      if(it != tmp_plq.end()) {
        //element in BTT found in PLQ
        int plq_id = DL1->return_plq_index(*it);
        if(plq_id != -1 && DL1->plq_vec[plq_id].tracking == 0) {
          //Delta changed for ld_ptr "i"; don't trigger load
          all_ptr_track = false;
        }
      }else {
        //element in BRR not found in PLQ
        all_ptr_found = false;
        //start tracking this ld_ptr "i" as its part of the current LD-BR slice
        if(lt_idx != -1) {
          //DL1->load_table_vec[lt_idx].tracking++;
          DL1->load_table_vec[lt_idx].lt_update_tracking(true);
        }
      }
    }
  }
  if(all_ptr_found) {
    for(int i = 0; i < DL1->plq_vec.size(); i++) {
      //clears sp.track and PLQ entry if x is present in PLQ but not in BTT
      auto it = std::find(btt_vec[btt_id].load_table_pointer.begin(), btt_vec[btt_id].load_table_pointer.end(), DL1->plq_vec[i].load_pointer);
      if(it != btt_vec[btt_id].load_table_pointer.end()) {
        //element in PLQ is found in BTT
      }else {
        //No need to track ld-ptr in ld_table and plq if this ld-ptr is not part of current LD-BR slice
        int lt_idx = DL1->return_load_table_index(DL1->plq_vec[i].load_pointer);
        if(lt_idx != -1) {
          DL1->load_table_vec[lt_idx].tracking = 0;
        }
#if 0
        DL1->plq_vec.erase(DL1->plq_vec.begin() + i);
        DL1->plq_vec.push_back(MemObj::pending_load_queue());
#endif
        std::vector<MemObj::pending_load_queue> plq;
        for(int j = 0; j < DL1->plq_vec.size(); j++) {
          if(j != i) {
            plq.push_back(DL1->plq_vec[i]);
          }
        }
        DL1->plq_vec.clear();
        DL1->plq_vec = plq;
        DL1->plq_vec.push_back(MemObj::pending_load_queue());
      }
    }
    if(all_ptr_track)
      return 1;
  }

  return 0;
}

#if 0
int OoOProcessor::return_plq_index(AddrType pc) {
  //if hit on load_table, update and move entry to LRU position
  for(int i = PLQ_SIZE - 1; i >= 0; i--) {
    if(pc == plq_vec[i].load_pointer) {
      return i;
    }
  }
  return -1;
}
#endif

DataType OoOProcessor::extract_load_immediate(AddrType pc) {
  AddrType raw_op = esesc_mem_read(pc);
  uint32_t d0 = (raw_op >> 2) & (~0U >> 27);
  uint64_t d1 = (int64_t)(raw_op << 51) >> 63;
  DataType d  = d1 | d0;
  //MSG("op=%llx data=%d", raw_op, d);
  return d;
}

#endif

void OoOProcessor::retire()
/* Try to retire instructions {{{1 */
{

#ifdef ENABLE_LDBP
  int64_t gclock = int64_t(clockTicks.getDouble());
  if(gclock != power_clock) {
    power_clock = gclock;
    if(power_save_mode_ctr <= (MAX_POWER_SAVE_MODE_CTR + 1)) {
      power_save_mode_ctr++;
    }
    if(power_save_mode_ctr >= MAX_POWER_SAVE_MODE_CTR) {
      //reset tables and power off
      ldbp_power_save_cycles.inc(true);
      tmp_power_clock++;
      //MSG("global=%d tmp_power_clock=%d", gclock, tmp_power_clock);
      if(power_save_mode_ctr == MAX_POWER_SAVE_MODE_CTR) {
        power_save_mode_table_reset();
      }
    }
  }
#endif
  // Pass all the ready instructions to the rrob
  while(!ROB.empty()) {
    DInst *dinst = ROB.top();
    uint64_t num_inflight_branches = 0;

    bool done = dinst->getClusterResource()->preretire(dinst, flushing);
    //AddrType ppc = dinst->getPC();
    //MSG("MV");
    GI(flushing && dinst->isExecuted(), done);
    if(!done) {
      break;
    }

#ifdef ENABLE_LDBP
#if 0
    //extracting Li's immediate value
    if(dinst->getPC() == 0x10400 || dinst->getPC() == 0x10402) {
      AddrType value = esesc_mem_read(dinst->getPC());
      uint32_t d0 = (value >> 2) & (~0U >> 27);
      uint64_t d1 = (int64_t)(value << 51) >> 63;
      DataType d  = d1 | d0;
      MSG("op=%llx data=%d", value, d);
    }
#endif
    if(dinst->getInst()->isLoad()) {

      num_loads.inc(true); //inc load counter
      DL1->hit_on_load_table(dinst, false);
      int lt_idx = DL1->return_load_table_index(dinst->getPC());
      if((lt_idx != -1)) {
        if(DL1->load_table_vec[lt_idx].conf > DL1->getLoadTableConf())
          num_ld_conf.inc(true);
        if(DL1->load_table_vec[lt_idx].data_conf > DL1->getLoadDataConf())
          num_ld_data_conf.inc(true);
      }
      rtt_load_hit(dinst);
    }

    //handle complex ALU in RTT
    if(dinst->getInst()->isComplex()) {
      RegType dst  = dinst->getInst()->getDst1();
      rtt_vec[dst].num_ops = NUM_OPS + 1;
      rtt_vec[dst].is_complex = true;
    }

    if(dinst->getInst()->isALU()) { //if ALU hit on ldbp_retire_table
      RegType alu_dst = dinst->getInst()->getDst1();
      RegType alu_src1 = dinst->getInst()->getSrc1();
      RegType alu_src2 = dinst->getInst()->getSrc2();
      if(alu_src1 == LREG_R0 && alu_src2 == LREG_R0) { //Load Immediate
        //DL1->hit_on_load_table(dinst, true);
        rtt_load_hit(dinst); //load_table_vec index is always 0 for Li; index 1 to LOAD_TABLE_SIZE is for other LDs
        //rtt_alu_hit(dinst);
      }else { // other ALUs
        rtt_alu_hit(dinst);
      }

    }

#if 1
    // condition to find a MV instruction
    if(dinst->getInst()->getOpcode() == iRALU && (dinst->getInst()->getSrc1() != LREG_R0 && dinst->getInst()->getSrc2() == LREG_R0)) {
      RegType alu_dst = dinst->getInst()->getDst1();
      rtt_alu_hit(dinst);
#if 0
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = true;
        ct_table[alu_dst].mv_src    = dinst->getInst()->getSrc1();
      }
#endif
    }else if(dinst->getInst()->getOpcode() == iRALU && (dinst->getInst()->getSrc1() == LREG_R0 && dinst->getInst()->getSrc2() != LREG_R0)) {
      RegType alu_dst = dinst->getInst()->getDst1();
      rtt_alu_hit(dinst);
#if 0
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = true;
        ct_table[alu_dst].mv_src    = dinst->getInst()->getSrc2();
      }
#endif
    }
#endif

    //if(dinst->getInst()->getOpcode() == iBALU_LBRANCH) {}
    if(dinst->getInst()->isBranch()) {
      num_br.inc(true);
      //compute num of inflight branches
      for(uint32_t i = 0; i < ROB.size(); i++) { //calculate num of inflight branches
        uint32_t pos   = ROB.getIDFromTop(i);
        DInst*  tmp_dinst = ROB.getData(pos);
        if(tmp_dinst->getInst()->isBranch() && (tmp_dinst->getPC() == dinst->getPC()))
          num_inflight_branches++;
      }
      inflight_branch = num_inflight_branches;
      dinst->setInflight(inflight_branch);

      if(dinst->isUseLevel3()) {
        power_save_mode_ctr = 0;
        //ldbp_power_mode_cycles.inc(true);
      }
      rtt_br_hit(dinst);

    }

#endif

    I(IFID.getMissDInst() != dinst);

    rROB.push(dinst);
    ROB.pop();
  }

  if(!ROB.empty() && ROB.top()->getStatsFlag()) {
    robUsed.sample(ROB.size(), true);
#ifdef TRACK_TIMELEAK
    int total_hit  = 0;
    int total_miss = 0;
    for(uint32_t i = 0; i < ROB.size(); i++) {
      uint32_t pos   = ROB.getIDFromTop(i);
      DInst *  dinst = ROB.getData(pos);

      if(!dinst->getStatsFlag())
        continue;
      if(!dinst->getInst()->isLoad())
        continue;
      if(dinst->isPerformed())
        continue;

      if(dinst->isFullMiss())
        total_miss++;
      else
        total_hit++;
    }
    avgPNRHitLoadSpec.sample(total_hit, true);
    avgPNRMissLoadSpec.sample(true, total_miss);
#endif
  }

  if(!rROB.empty()) {
    rrobUsed.sample(rROB.size(), rROB.top()->getStatsFlag());

#ifdef ESESC_CODEPROFILE
    if(rROB.top()->getStatsFlag()) {
      if(codeProfile_trigger <= clockTicks.getDouble()) {
        DInst *dinst = rROB.top();

        codeProfile_trigger = clockTicks.getDouble() + 121;

        double wt    = dinst->getIssuedTime() - dinst->getRenamedTime();
        double et    = dinst->getExecutedTime() - dinst->getIssuedTime();
        bool   flush = dinst->isBranchMiss();

        codeProfile.sample(rROB.top()->getPC(), nCommitted.getDouble(), clockTicks.getDouble(), wt, et, flush, dinst->isPrefetch(), dinst->isBranchMiss_level1(), dinst->isBranchMiss_level2(), dinst->isBranchMiss_level3(), dinst->isBranchHit_level1(), dinst->isBranchHit_level2(), dinst->isBranchHit_level3(), dinst->isBranch_hit2_miss3(), dinst->isBranch_hit3_miss2());
      }
    }
#endif
  }

  for(uint16_t i = 0; i < RetireWidth && !rROB.empty(); i++) {
    DInst *dinst = rROB.top();

    if((dinst->getExecutedTime() + RetireDelay) >= globalClock) {
#if 0
      if (rROB.size()>8) {
        dinst->getInst()->dump("not ret");
        printf("----------------------\n");
        dumpROB();
      }
#endif
      break;
    }

    I(dinst->getCluster());

    bool done = dinst->getCluster()->retire(dinst, flushing);
    if(!done)
      break;

#if 0
    static int conta=0;
    if ((globalClock-dinst->getExecutedTime())>500)
      conta++;
    if (conta > 1000) {
      dinst->getInst()->dump("not ret");
      conta = 0;
      dumpROB();
    }
#endif

    FlowID fid = dinst->getFlowId();
    if(dinst->isReplay()) {
      flushing     = true;
      flushing_fid = fid;
    }

#ifdef FETCH_TRACE
    // Call eint->markTrace
#endif

    nCommitted.inc(!flushing && dinst->getStatsFlag());

#ifdef ESESC_BRANCHPROFILE
    if(dinst->getInst()->isBranch() && dinst->getStatsFlag()) {
      codeProfile.sample(dinst->getPC(), dinst->getID(), 0, dinst->isBiasBranch() ? 1.0 : 0, 0, dinst->isBranchMiss(), dinst->isPrefetch(), dinst->getLBType(), dinst->isBranchMiss_level1(), dinst->isBranchMiss_level2(), dinst->isBranchMiss_level3(), dinst->isBranchHit_level1(), dinst->isBranchHit_level2(), dinst->isBranchHit_level3(), dinst->isBranch_hit2_miss3(), dinst->isBranch_hit3_miss2(), dinst->isTrig_ld1_pred(), dinst->isTrig_ld1_unpred(), dinst->isTrig_ld2_pred(), dinst->isTrig_ld2_unpred(), dinst->get_trig_ld_status());
      AddrType p = dinst->getPC();
      //MSG("BR_PROFILE clk=%u brpc=%llx br_miss=%d\n", globalClock, dinst->getPC(), dinst->isBranchMiss());
    }
#endif
#if 0
    if (dinst->getPC() == 0x1100c || dinst->getPC() == 0x11008)
      MSG("TR %8llx %lld\n",dinst->getPC(), dinst->getAddr());
#endif
#ifdef ESESC_TRACE2
    MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d   %lld %lld %lld %lld %lld", dinst->getID(), dinst->getPC(),
        dinst->getInst()->getDst1(), dinst->getInst()->getDst2(), dinst->getInst()->getSrc1(), dinst->getInst()->getOpcode(),
        dinst->getInst()->getSrc2(), dinst->getFetchedTime(), dinst->getRenamedTime(), dinst->getIssuedTime(),
        dinst->getExecutedTime(), globalClock);
#endif
#ifdef ESESC_TRACE
    MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d   %lld %lld %lld %lld %lld", dinst->getID(), dinst->getPC(),
        dinst->getInst()->getDst1(), dinst->getInst()->getDst2(), dinst->getInst()->getSrc1(), dinst->getInst()->getOpcode(),
        dinst->getInst()->getSrc2(), dinst->getFetchedTime() - globalClock, dinst->getRenamedTime() - globalClock,
        dinst->getIssuedTime() - globalClock, dinst->getExecutedTime() - globalClock, globalClock - globalClock);
#endif
    // sjeng 2nd: if (dinst->getPC() == 0x1d46c || dinst->getPC() == 0x1d462)
#if 0
    if (dinst->getPC() == 0x10c96 || dinst->getPC() == 0x10c9a || dinst->getPC() == 0x10c9e)
      MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d  %lld %lld %lld", dinst->getID(), dinst->getPC(),
          dinst->getInst()->getDst1(), dinst->getInst()->getDst2(), dinst->getInst()->getSrc1(), dinst->getInst()->getOpcode(),
          dinst->getInst()->getSrc2(), dinst->getData(), dinst->getData2(), dinst->getAddr());
#endif

#if 0
    dinst->dump("RT ");
    fprintf(stderr,"\n");
#endif

#ifdef WAVESNAP_EN
//updading wavesnap instruction windows
if(SINGLE_WINDOW) {
  if(WITH_SAMPLING) {
    if(!flushing && dinst->getStatsFlag()) {
      snap->update_single_window(dinst, (uint64_t)globalClock);
    }
  } else {
    snap->update_single_window(dinst, (uint64_t)globalClock);
  }
} else {
  if(WITH_SAMPLING) {
    if(dinst->getStatsFlag()) {
      snap->update_window(dinst, (uint64_t)globalClock);
    }
  } else {
    snap->update_window(dinst, (uint64_t)globalClock);
  }

}
#endif
    if(dinst->getInst()->hasDstRegister())
      nTotalRegs++;

    if(!dinst->getInst()->isStore()) // Stores can perform after retirement
      I(dinst->isPerformed());

    if(dinst->isPerformed()) // Stores can perform after retirement
      dinst->destroy(eint);
    else {
      eint->reexecuteTail(fid);
    }

    if(last_serialized == dinst)
      last_serialized = 0;
    if(last_serializedST == dinst)
      last_serializedST = 0;

    rROB.pop();
  }
}
/* }}} */

void OoOProcessor::replay(DInst *target)
/* trigger a processor replay {{{1 */
{
  if(serialize_for)
    return;

  I(serialize_for <= 0);
  // Same load can be marked by several stores in a OoO core : I(replayID != target->getID());
  I(target->getInst()->isLoad());

  if(!MemoryReplay) {
    return;
  }
  target->markReplay();

  if(replayID < target->getID())
    replayID = target->getID();

  if(replayRecovering)
    return;
  replayRecovering = true;

  // Count the # instructions wasted
  size_t fetch2rename = 0;
  fetch2rename += (InstQueueSize - spaceInInstQueue);
  fetch2rename += pipeQ.pipeLine.size();

  nReplayInst.sample(fetch2rename + ROB.size(), target->getStatsFlag());
}
/* }}} */

void OoOProcessor::dumpROB()
// {{{1 Dump rob statistics
{
#if 0
  uint32_t size = ROB.size();
  fprintf(stderr,"ROB: (%d)\n",size);

  for(uint32_t i=0;i<size;i++) {
    uint32_t pos = ROB.getIDFromTop(i);

    DInst *dinst = ROB.getData(pos);
    dinst->dump("");
  }

  size = rROB.size();
  fprintf(stderr,"rROB: (%d)\n",size);
  for(uint32_t i=0;i<size;i++) {
    uint32_t pos = rROB.getIDFromTop(i);

    DInst *dinst = rROB.getData(pos);
    if (dinst->isReplay())
      printf("-----REPLAY--------\n");
    dinst->dump("");
  }
#endif
}

bool OoOProcessor::loadIsSpec(){
  
  std::vector<double> mem_unresolved;
  std::vector<double> br_unresolved;
  std::vector<double> div_unresolved;
  int robSize=ROB.size();
  if(robSize>0){
    for(uint32_t i=0;i<robSize;i++) {
      uint32_t pos = ROB.getIDFromTop(i);
      DInst *dinst = ROB.getData(pos);
      if(dinst->getInst()->isMemory()){
        if(!dinst->isExecuting()){
         mem_unresolved.push_back(pos);
         //fprintf(stderr,"Mem is at Unresolved and vector_value is (%f)\n",  mem_unresolved.size());
       }
     }else if(dinst->getInst()->isBranch()){
       if(!dinst->isExecuted()){
         br_unresolved.push_back(pos);
       }
     }else if(dinst->getInst()->getOpcode() == iCALU_FPDIV|| dinst->getInst()->getOpcode() == iCALU_DIV){
       if(!dinst->isExecuted()){
         div_unresolved.push_back(pos);
       }
     }
    }
  }

  if(mem_unresolved.size()>0 || br_unresolved.size()>0 || div_unresolved.size()>0)
  { //printf("SPEC_added\n");
    return true;
  } else {
   // printf("SAFE_added\n");
    return false;
  }
}

// 1}}}
