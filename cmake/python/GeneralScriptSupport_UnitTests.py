#################################################
# Unit testing code for GeneralScriptSupport.py #
#################################################

from GeneralScriptSupport import *
import unittest


utilsDir = getScriptBaseDir()+"/utils"


class testGeneralScriptSupport(unittest.TestCase):


  def setUp(self):
    None


  def test_normalizePath_1(self):
    #print "\ntest_normalizePath:"
    pathIn = "./aaa/bbb"
    pathOut = normalizePath(pathIn)
    pathOut_expected = "aaa/bbb"
    self.assertEqual(pathOut, pathOut_expected)


  def test_arrayToFormattedString(self):
    #print "\ntest_normalizePath:"
    array = [ 'aaa', 'bbb', 'cc' ]
    arrayAsStr = arrayToFormattedString(array, "  ")
    arrayAsStr_expected = "  [\n    'aaa',\n    'bbb',\n    'cc'\n  ]\n"
    self.assertEqual(arrayAsStr, arrayAsStr_expected)


  def test_getRelativePathFrom1to2_not_exclusive(self):
    #print "\ntest_getRelativePathFrom1to2_not_exclusive:"
    absPath1 = "/a/b/f/g"
    absPath2 = "/a/b/c/d"
    relPath1to2 = getRelativePathFrom1to2(absPath1, absPath2)
    relPath1to2_expected = "../../c/d"
    self.assertEqual(relPath1to2, relPath1to2_expected)


  def test_getRelativePathFrom1to2_path1_in_path2_2_deep(self):
    #print "\ntest_getRelativePathFrom1to2_path1_in_path2_2_deep:"
    absPath1 = "/a/b"
    absPath2 = "/a/b/c/d"
    relPath1to2 = getRelativePathFrom1to2(absPath1, absPath2)
    relPath1to2_expected = "./c/d"
    self.assertEqual(relPath1to2, relPath1to2_expected)


  def test_getRelativePathFrom1to2_path1_in_path2_1_deep(self):
    #print "\ntest_getRelativePathFrom1to2_path1_in_path2_1_deep:"
    absPath1 = "/somebasedir/Trilinos/dev/flat_headers"
    absPath2 = "/somebasedir/Trilinos/dev"
    relPath1to2 = getRelativePathFrom1to2(absPath1, absPath2)
    relPath1to2_expected = "../."
    self.assertEqual(relPath1to2, relPath1to2_expected)


  def test_getRelativePathFrom1to2_path2_in_path1(self):
    #print "\ntest_getRelativePathFrom1to2_path2_in_path1:"
    absPath1 = "/a/b/c/d"
    absPath2 = "/a/b"
    relPath1to2 = getRelativePathFrom1to2(absPath1, absPath2)
    relPath1to2_expected = "../../."
    self.assertEqual(relPath1to2, relPath1to2_expected)


  def test_getRelativePathFrom1to2_path1_equals_path2(self):
    #print "\ntest_getRelativePathFrom1to2_path1_equals_path2:"
    absPath1 = "/a/b"
    absPath2 = "/a/b"
    relPath1to2 = getRelativePathFrom1to2(absPath1, absPath2)
    relPath1to2_expected = "."
    self.assertEqual(relPath1to2, relPath1to2_expected)


  def test_expandDirsDict(self):

    dirsDict = {
      './TPLs_src/Trilinos/dev/packages/thyra/adapters/epetraext/src/model_evaluator' : 0,
      './TPLs_src/Trilinos/dev/packages/sacado/src/pce' : 0,
      './TPLs_src/Trilinos/dev/packages/ml/src/Operator' : 0
      }
    expandDirsDict(dirsDict)
    expandedDirsList = dirsDict.keys()
    expandedDirsList.sort()
    #print "\nexpandedDirsList =\n", '\n'.join(expandedDirsList)

    expandedDirsList_expected = [
      ".",
      "./TPLs_src",
      "./TPLs_src/Trilinos",
      "./TPLs_src/Trilinos/dev",
      "./TPLs_src/Trilinos/dev/packages",
      "./TPLs_src/Trilinos/dev/packages/ml",
      "./TPLs_src/Trilinos/dev/packages/ml/src",
      "./TPLs_src/Trilinos/dev/packages/ml/src/Operator",
      "./TPLs_src/Trilinos/dev/packages/sacado",
      "./TPLs_src/Trilinos/dev/packages/sacado/src",
      "./TPLs_src/Trilinos/dev/packages/sacado/src/pce",
      "./TPLs_src/Trilinos/dev/packages/thyra",
      "./TPLs_src/Trilinos/dev/packages/thyra/adapters",
      "./TPLs_src/Trilinos/dev/packages/thyra/adapters/epetraext",
      "./TPLs_src/Trilinos/dev/packages/thyra/adapters/epetraext/src",
      "./TPLs_src/Trilinos/dev/packages/thyra/adapters/epetraext/src/model_evaluator"
      ]

    self.assertEqual( expandedDirsList, expandedDirsList_expected )


  def test_runSysCmndInteface_pass(self):
    self.assertEqual(0, runSysCmndInterface(utilsDir+"/return_input.py 0"))


  def test_runSysCmndInteface_fail(self):
    self.assertEqual(1, runSysCmndInterface(utilsDir+"/return_input.py 1"))


  def test_runSysCmndInteface_rtnOutput_pass(self):
    self.assertEqual(("junk\n", None), runSysCmndInterface("echo junk", rtnOutput=True))


  def test_runSysCmndInteface_rtnOutput_fail(self):
    (output, rtnCode) = runSysCmndInterface(utilsDir+"/return_input.py 5", rtnOutput=True)
    self.assertNotEqual(rtnCode, 0) # Does not return the right rtnCode!


  def test_SysCmndInterceptor_isFallThroughCmnd(self):
    sci = SysCmndInterceptor()
    self.assertEqual(sci.hasInterceptedCmnds(), False)
    sci.setFallThroughCmndRegex("eg log.*")
    sci.setFallThroughCmndRegex("ls.*")
    self.assertEqual(sci.hasInterceptedCmnds(), False)
    self.assertEqual(sci.isFallThroughCmnd("eg log"), True)
    self.assertEqual(sci.isFallThroughCmnd("eg log origin.."), True)
    self.assertEqual(sci.isFallThroughCmnd("eg pull"), False)
    self.assertEqual(sci.isFallThroughCmnd("ls"), True)
    self.assertEqual(sci.isFallThroughCmnd("ls dogs"), True)
    self.assertEqual(sci.isFallThroughCmnd("mkdir cats"), False)


  def test_SysCmndInterceptor_nextInterceptedCmndStruct(self):
    sci = SysCmndInterceptor()
    self.assertEqual(sci.hasInterceptedCmnds(), False)
    sci.setInterceptedCmnd("eg commit", 0)
    self.assertEqual(sci.hasInterceptedCmnds(), True)
    self.assertRaises(Exception, sci.nextInterceptedCmndStruct, "eg pull")


  def test_SysCmndInterceptor_readCmndFile_01(self):
    sci = SysCmndInterceptor()
    sci.readCommandsFromStr(
"""
FT: eg log.*
FT: ls .*
IT: eg log; 0; 'good log'
IT: eg frog; 3; 'bad frog'
IT: ./do-configure; 5; ''
"""
    )
    self.assertEqual(["eg log.*", "ls .*"], sci.getFallThroughCmndRegexList())
    self.assertEqual( str(InterceptedCmndStruct("eg log", 0, "good log")),
      str(sci.getInterceptedCmndStructList()[0]) )
    self.assertEqual( str(InterceptedCmndStruct("eg frog", 3, "bad frog")),
      str(sci.getInterceptedCmndStructList()[1]) )
    self.assertEqual( str(InterceptedCmndStruct("./do-configure", 5, "")),
      str(sci.getInterceptedCmndStructList()[2]) )


  def test_runSysCmndInterface_fall_through(self):
    try:
      g_sysCmndInterceptor.setFallThroughCmndRegex("echo .+")
      self.assertEqual(3, runSysCmndInterface(utilsDir+"/return_input.py 3"))
    finally:
      g_sysCmndInterceptor.clear()


  def test_runSysCmndInterface_intercept_rtnCode_01(self):
    try:
      g_sysCmndInterceptor.setFallThroughCmndRegex("echo .+")
      g_sysCmndInterceptor.setInterceptedCmnd("eg log", 3)
      g_sysCmndInterceptor.setInterceptedCmnd("eg frog", 5)
      g_sysCmndInterceptor.setAllowExtraCmnds(False)
      self.assertEqual(3, runSysCmndInterface("eg log"))
      self.assertEqual(("dummy1\n", None),
        runSysCmndInterface("echo dummy1", rtnOutput=True)) # Fall through!
      self.assertEqual(5, runSysCmndInterface("eg frog"))
      self.assertEqual(g_sysCmndInterceptor.hasInterceptedCmnds(), False)
      self.assertRaises(Exception, runSysCmndInterface, utilsDir+"/return_input.py 2")
      self.assertEqual(("dummy2\n", None),
        runSysCmndInterface("echo dummy2", rtnOutput=True)) # Fall through!
      g_sysCmndInterceptor.setAllowExtraCmnds(True)
      self.assertEqual(4, runSysCmndInterface(utilsDir+"/return_input.py 4")) # Fall through!
    finally:
      g_sysCmndInterceptor.clear()


  def test_runSysCmndInterface_intercept_rtnOutput_01(self):
    try:
      g_sysCmndInterceptor.setInterceptedCmnd("eg log", 3, "bad log\n")
      g_sysCmndInterceptor.setInterceptedCmnd("eg frog", 5, "good frog\n")
      g_sysCmndInterceptor.setInterceptedCmnd("eg blog", 7) # No output defined
      g_sysCmndInterceptor.setAllowExtraCmnds(False)
      self.assertEqual(("bad log\n", 3), runSysCmndInterface("eg log", rtnOutput=True))
      self.assertEqual(("good frog\n", 5), runSysCmndInterface("eg frog", rtnOutput=True))
      self.assertRaises(Exception, runSysCmndInterface, "eg blog", rtnOutput=True)
    finally:
      g_sysCmndInterceptor.clear()


  def test_runSysCmndInterface_intercept_outFile_01(self):
    try:
      g_sysCmndInterceptor.setInterceptedCmnd("eg log", 3, "bad log\n")
      self.assertEqual(3, runSysCmndInterface("eg log", outFile="eg_log.out"))
      self.assertEqual("bad log\n", readStrFromFile("eg_log.out"))
    finally:
      g_sysCmndInterceptor.clear()


  def test_runSysCmnd_intercept_01(self):
    try:
      g_sysCmndInterceptor.setFallThroughCmndRegex("echo .+")
      g_sysCmndInterceptor.setInterceptedCmnd("eg log", 3)
      g_sysCmndInterceptor.setInterceptedCmnd("eg frog", 5)
      g_sysCmndInterceptor.setAllowExtraCmnds(False)
      self.assertEqual(3, runSysCmnd("eg log", throwExcept=False))
      self.assertRaises(Exception, runSysCmnd, "eg frog")
      self.assertEqual(g_sysCmndInterceptor.hasInterceptedCmnds(), False)
      self.assertRaises(Exception, runSysCmnd, utilsDir+"/return_input.py 1", throwExcept=False)
      g_sysCmndInterceptor.setAllowExtraCmnds(True)
      self.assertRaises(Exception, runSysCmnd, utilsDir+"/return_input.py 3") # Fall through!
    finally:
      g_sysCmndInterceptor.clear()


  def test_getCmndOutput_intercept_01(self):
    try:
      g_sysCmndInterceptor.setFallThroughCmndRegex("echo .+")
      g_sysCmndInterceptor.setInterceptedCmnd("eg log", 0, "good log\n")
      g_sysCmndInterceptor.setInterceptedCmnd("eg frog", 0, "bad frog\n")
      g_sysCmndInterceptor.setInterceptedCmnd("eg blog", 2, "who cares\n")
      g_sysCmndInterceptor.setAllowExtraCmnds(False)
      self.assertEqual("good log", getCmndOutput("eg log", True))
      self.assertEqual("bad frog", getCmndOutput("eg frog", True, False))
      self.assertRaises(Exception, getCmndOutput, "eg blog", True)
    finally:
      g_sysCmndInterceptor.clear()


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(testGeneralScriptSupport))
    return suite


if __name__ == '__main__':
  unittest.main()