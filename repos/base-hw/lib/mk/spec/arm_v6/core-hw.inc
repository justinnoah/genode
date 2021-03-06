#
# \brief  Build config for Genodes core process
# \author Martin Stein
# \author Stefan Kalkowski
# \date   2012-10-04
#

# add include paths
REP_INC_DIR += src/core/spec/arm_v6

# add C++ sources
SRC_CC += spec/arm_v6/perf_counter.cc
SRC_CC += kernel/vm_thread_off.cc
SRC_CC += kernel/cpu_up.cc
SRC_CC += kernel/lock.cc

SRC_S  += spec/arm/vfpv2.s

# include less specific configuration
include $(call select_from_repositories,lib/mk/spec/arm/core-hw.inc)
