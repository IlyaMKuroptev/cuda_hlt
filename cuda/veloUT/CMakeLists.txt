
file(GLOB veloUT_tracking "PrVeloUT/src/*cu")

include_directories(common/include)
include_directories(PrVeloUT/include)
include_directories(../../main/include)
include_directories(../velo/common/include)

cuda_add_library(VeloUT STATIC
  ${veloUT_tracking}
)

if ( MC_CHECK )
  target_compile_definitions(VeloUT PUBLIC MC_CHECK)
endif()
