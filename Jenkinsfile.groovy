node {
   stage 'git'
   checkout scm
   sh 'git submodule update --init'

   stage 'pre-analysis'
   sh 'cppcheck --xml-version=2 --enable=all --std=c++11 src/*.cpp 2> cppcheck_report.xml'
   sh 'sloccount --duplicates --wide --details src > sloccount.sc'
   sh 'cccc src/*.cpp || true'

   stage 'sonar'
   sh '/opt/sonar-runner/bin/sonar-runner'
}
