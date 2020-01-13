#!/usr/bin/env groovy
// This shared library is available at https://github.com/ROCmSoftwarePlatform/rocJENKINS/
@Library('rocJenkins') _

// This is file for internal AMD use.
// If you are interested in running your own Jenkins, please raise a github issue for assistance.

import com.amd.project.*
import com.amd.docker.*
import java.nio.file.Path


rocALUTIONCI:
{

    def rocalution = new rocProject('rocALUTION', 'Extended')
    // customize for project
    rocalution.paths.build_command = './install.sh -c'
    rocalution.compiler.compiler_name = 'c++'
    rocalution.compiler.compiler_path = 'c++'

    // Define test architectures, optional rocm version argument is available
    def nodes = new dockerNodes(['gfx900 && ubuntu', 'gfx906 && centos7', 'gfx906 && sles'], rocalution)

    def commonGroovy

    boolean formatCheck = true

    def compileCommand =
    {
        platform, project->

        commonGroovy = load "${project.paths.project_src_prefix}/.jenkins/Common.groovy"
        commonGroovy.runCompileCommand(platform, project)
    }

    def testCommand =
    {
        platform, project->

        def gfilter = "*nightly*"
        commonGroovy.runTestCommand(platform, project, gfilter)
    }

    def packageCommand =
    {
        platform, project->

        commonGroovy.runPackageCommand(platform, project)
    }

    buildProject(rocalution, formatCheck, nodes.dockerArray, compileCommand, testCommand, packageCommand)

}
