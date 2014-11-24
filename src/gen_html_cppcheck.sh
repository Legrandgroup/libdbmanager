#/bin/bash --login
cppcheck --enable=all --xml-version=2 . 2> output.xml
cppcheck-htmlreport --file=output.xml --title=LibDBManager --report-dir=output/ --source-dir=.
