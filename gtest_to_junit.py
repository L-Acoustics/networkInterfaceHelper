import xml.etree.ElementTree as ET

def convert_gtest_to_junit(gtest_xml, junit_xml):
    tree = ET.parse(gtest_xml)
    root = tree.getroot()

    testsuites = ET.Element("testsuites")
    for testsuite in root.findall("testsuite"):
        new_testsuite = ET.Element("testsuite")
        new_testsuite.attrib = testsuite.attrib
        for testcase in testsuite.findall("testcase"):
            new_testcase = ET.Element("testcase")
            new_testcase.attrib = testcase.attrib
            for failure in testcase.findall("failure"):
                new_failure = ET.Element("failure")
                new_failure.attrib = failure.attrib
                new_failure.text = failure.text
                new_testcase.append(new_failure)
            new_testsuite.append(new_testcase)
        testsuites.append(new_testsuite)

    tree = ET.ElementTree(testsuites)
    tree.write(junit_xml)

convert_gtest_to_junit("test_results.xml", "junit_output.xml")

