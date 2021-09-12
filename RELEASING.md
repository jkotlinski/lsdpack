## Release Guide for Maintainers

 - Update CHANGELOG.md with new version information
 - Update CMAKE_PACKAGE_VERSION in CMakeLists.txt
 - Using Cygwin: `sh build-win.sh` to produce bin/lsdpack-VERSION-win64.zip
 - `git commit -a`
 - `git tag v#.#.#` (use new version, e.g. v1.2.3)
 - `git push`
 - `git push --tag`
 - Create new release using Github UI, uploading bin/*.zip
