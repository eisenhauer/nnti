#########################################################
# Unit testing code for TrilinosPackageFilePathUtils.py #
######################################################### 


from TrilinosPackageFilePathUtils import *
import unittest


trilinosDependencies = getTrilinosDependenciesFromXmlFile(defaultTrilinosDepsXmlInFile)
#print "\ntrilinosDependencies:\n", trilinosDependencies


updateOutputStr = """
? packages/triutils/doc/html
M cmake/python/checkin-test.py
M cmake/python/dump-cdash-deps-xml-file.py
A packages/nox/src/dummy.C
P packages/stratimikos/dummy.blah
M packages/thyra/src/Thyra_ConfigDefs.hpp
M packages/thyra/CMakeLists.txt
"""

updateOutputList = updateOutputStr.split("\n")


class testTrilinosPackageFilePathUtils(unittest.TestCase):


  def test_getPackageNameFromPath_01(self):
    self.assertEqual(
      getPackageNameFromPath( trilinosDependencies, 'packages/teuchos/CMakeLists.txt' ), 'Teuchos' )


  def test_getPackageNameFromPath_02(self):
    self.assertEqual(
      getPackageNameFromPath( trilinosDependencies, 'packages/thyra/src/blob.cpp' ), 'Thyra' )


  def test_getPackageNameFromPath_noMatch(self):
    self.assertEqual(
      getPackageNameFromPath( trilinosDependencies, 'packages/blob/blob' ), '' )


  def test_extractFilesListMatchingPattern_01(self):

    modifedFilesList = extractFilesListMatchingPattern( updateOutputList,
      re.compile(r"^[MA] (.+)$") )

    modifedFilesList_expected = \
      [
        "cmake/python/checkin-test.py",
        "cmake/python/dump-cdash-deps-xml-file.py",
        "packages/nox/src/dummy.C",
        "packages/thyra/src/Thyra_ConfigDefs.hpp",
        "packages/thyra/CMakeLists.txt",
      ]

    self.assertEqual( modifedFilesList, modifedFilesList_expected )


  def test_getPackgesListFromFilePathsList_01(self):

    filesList = extractFilesListMatchingPattern( updateOutputList,
      re.compile(r"^[AMP?] (.+)$") )
    
    packagesList = getPackgesListFromFilePathsList( trilinosDependencies, filesList )

    packagesList_expected = [u"Triutils", u"", u"NOX", u"Stratimikos", u"Thyra"]

    self.assertEqual( packagesList, packagesList_expected )


  def test_getPackageCheckinEmailAddressesListFromFilePathsList_01(self):

    filesList = extractFilesListMatchingPattern( updateOutputList,
      re.compile(r"^[AMP?] (.+)$") )
    
    packagesList = getPackageCheckinEmailAddressesListFromFilePathsList(
      trilinosDependencies, filesList )

    packagesList_expected = [
      u"triutils-checkins@software.sandia.gov",
      u"trilinos-checkins@software.sandia.gov",
      u"nox-checkins@software.sandia.gov",
      u"stratimikos-checkins@software.sandia.gov",
      u"thyra-checkins@software.sandia.gov"
      ]

    self.assertEqual( packagesList, packagesList_expected )


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(testTrilinosPackageFilePathUtils))
    return suite


if __name__ == '__main__':
  unittest.main()
