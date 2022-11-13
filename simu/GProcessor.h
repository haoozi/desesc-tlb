// See LICENSE for details.

#pragma once

//#define WAVESNAP_EN

#include <stdint.h>

// Generic Processor Interface.
//
// This class is a generic interface for Processors. It has been
// design for Traditional and SMT processors in mind. That's the
// reason why it manages the execution engine (RDEX).

#include "Cluster.h"
#include "ClusterManager.h"
#include "FastQueue.h"
#include "LSQ.h"
#include "Pipeline.h"
#include "Prefetcher.h"
#include "Resource.h"
#include "estl.h"

#include "callback.hpp"
#include "execute_engine.hpp"
#include "stats.hpp"
#include "iassert.hpp"
#include "instruction.hpp"
#include "snippets.hpp"
#include "store_buffer.hpp"
#include "wavesnap.hpp"

class GMemorySystem;
class BPredictor;


class GProcessor : public Execute_engine {
private:
protected:

  const int32_t FetchWidth;
  const int32_t IssueWidth;
  const int32_t RetireWidth;
  const int32_t RealisticWidth;
  const int32_t InstQueueSize;
  const size_t  MaxROBSize;

  Hartid_t       maxFlows;
  GMemorySystem *memorySystem;

  std::shared_ptr<StoreSet>     storeset;
  std::shared_ptr<Prefetcher>   prefetcher;
  std::shared_ptr<Store_buffer> scb;

  FastQueue<Dinst *> rROB;  // ready/retiring/executed ROB
  FastQueue<Dinst *> ROB;

  uint32_t smt;      // 1...
  uint32_t smt_ctx;  // 0... smt_ctx = cpu_id % smt

  // BEGIN  Statistics
  std::array<std::unique_ptr<Stats_cntr>,MaxStall> nStall;
  std::array<std::unique_ptr<Stats_cntr>,iMAX    > nInst;

  // OoO Stats
  Stats_avg  rrobUsed;
  Stats_avg  robUsed;
  Stats_avg  nReplayInst;
  Stats_cntr nCommitted;  // committed instructions

  // "Lack of Retirement" Stats
  Stats_cntr noFetch;
  Stats_cntr noFetch2;

  // END Statistics

  uint64_t lastReplay;

  // Construction
  void buildInstStats(const std::string &txt);
  void buildUnit(const std::string &clusterName, GMemorySystem *ms, Cluster *cluster, Opcode type);

  GProcessor(GMemorySystem *gm, Hartid_t i);
  int32_t issue(PipeQueue &pipeQ);

  //virtual void       fetch(Hartid_t fid)     = 0;
  virtual StallCause add_inst(Dinst *dinst) = 0;

public:
#ifdef WAVESNAP_EN
  std::unique_ptr<Wavesnap> snap;
#endif
  virtual ~GProcessor();

  virtual void   executing(Dinst *dinst) = 0;
  virtual void   executed(Dinst *dinst)  = 0;
  virtual LSQ   *getLSQ()                = 0;
  virtual bool   is_nuking()             = 0;
  virtual bool   isReplayRecovering()    = 0;
  virtual Time_t getReplayID()           = 0;

  virtual void replay(Dinst *target){ (void)target; };  // = 0;

  bool isROBEmpty() const { return (ROB.empty() && rROB.empty()); }
  int  getROBsize() const { return (ROB.size() + rROB.size()); }
  bool isROBEmptyOnly() const { return ROB.empty(); }

  int getROBSizeOnly() const { return ROB.size(); }

  uint32_t getIDFromTop(int position) const { return ROB.getIDFromTop(position); }
  Dinst   *getData(uint32_t position) const { return ROB.getData(position); }

  // Returns the maximum number of flows this processor can support
  Hartid_t getMaxFlows(void) const { return maxFlows; }

  void report(const std::string &str);

  std::shared_ptr<StoreSet>     ref_SS()         { return storeset;   }
  std::shared_ptr<Prefetcher>   ref_prefetcher() { return prefetcher; }
  std::shared_ptr<Store_buffer> ref_SCB()        { return scb;        }
};
