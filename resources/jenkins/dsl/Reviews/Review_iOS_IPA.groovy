import common.Review
import static common.Constants.strip

def j = new Review
	(
		name: 'iOS_IPA',
		libraries: ['iOS'],
		label: 'iOS',
		artifacts: 'build/*.ipa,build/*.zip'
	).generate(this)


j.with
{
	steps
	{
		shell('cd source; python resources/jenkins/import.py')

		shell('security unlock-keychain \${KEYCHAIN_CREDENTIALS} \${HOME}/Library/Keychains/login.keychain-db')

		shell(strip('''\
			cd build;
			cmake -Werror=dev ../source
			-DCMAKE_BUILD_TYPE=MinSizeRel
			-DCMAKE_PREFIX_PATH=\${WORKSPACE}/libs/build/dist
			-DCMAKE_TOOLCHAIN_FILE=../source/cmake/iOS.toolchain.cmake
			-GXcode
			'''))

		shell('cd build; xcodebuild -configuration MinSizeRel ARCHS=arm64')
		shell('cd build; xcodebuild -target ipa -configuration MinSizeRel')
	}
}
