otb_module_test()

set(AppSamplingTests
  AppSamplingTestDriver.cxx
  otbMaskedIteratorDecorator.cxx
)

add_executable(otbMaskedIteratorTestDriver ${AppSamplingTests})
target_link_libraries(otbMaskedIteratorTestDriver ${AppSampling-Test_LIBRARIES})
otb_module_target_label(otbMaskedIteratorTestDriver)

otb_test_application(NAME otbSampling
                     APP Convert
                     OPTIONS -in ${INPUTDATA}/QB_Toulouse_Ortho_XS.tif
                             -out ${TEMP}/apTvUtConvertBasicOutput.tif
                     VALID   --compare-image ${NOTOL}
                             ${INPUTDATA}/apTvUtConvertBasicOutput.tif
                       ${TEMP}/apTvUtConvertBasicOutput.tif
)

otb_add_test(NAME Test1MaskedIteratorDecorator COMMAND otbMaskedIteratorTestDriver
  otbMaskedIteratorDecoratorNew)
