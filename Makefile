all : build/makecapk.apk 

NDK=$(HOME)/.local/Android/ndk-bundle
BUILD_TOOLS=$(HOME)/.local/Android/build-tools/30.0.1
ANDROIDSDK=$(HOME)/.local/Android
ANDROIDVERSION=28
ANDROIDTARGET=28
APKFILE=balls.apk
AAPT:=$(BUILD_TOOLS)/aapt

TARGETS += build/makecapk/lib/arm64-v8a/libballs.so

STOREPASS?=password
DNAME:="CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB"
KEYSTOREFILE:=build/my-release-key.keystore
ALIASNAME?=standkey

keystore : $(KEYSTOREFILE)

$(KEYSTOREFILE) :
	keytool -genkey -v -keystore $(KEYSTOREFILE) -alias $(ALIASNAME) -keyalg RSA -keysize 2048 -validity 10000 -storepass $(STOREPASS) -keypass $(STOREPASS) -dname $(DNAME)

build/makecapk/lib/arm64-v8a/libballs.so :
	mkdir -p build/makecapk/lib/arm64-v8a
	cp build/src/libballs.so build/makecapk/lib/arm64-v8a/

build/makecapk.apk : $(TARGETS)
	mkdir -p build/makecapk/
	cp -r assets build/makecapk/
	rm -rf build/temp.apk
	$(AAPT) package -f -F build/temp.apk -I $(ANDROIDSDK)/platforms/android-$(ANDROIDVERSION)/android.jar -M AndroidManifest.xml -S res -A build/makecapk/assets -v --target-sdk-version $(ANDROIDTARGET)
	unzip -o build/temp.apk -d build/makecapk
	rm -rf build/makecapk.apk
	cd build/makecapk && zip -D9r ../makecapk.apk .
	jarsigner -sigalg SHA1withRSA -digestalg SHA1 -verbose -keystore $(KEYSTOREFILE) -storepass $(STOREPASS) build/makecapk.apk $(ALIASNAME)
	rm -rf build/$(APKFILE)
	$(BUILD_TOOLS)/zipalign -v 4 build/makecapk.apk build/$(APKFILE)
	rm -rf build/temp.apk
	rm -rf build/makecapk.apk

clean :
	rm -rf build/temp.apk build/makecapk.apk build/makecapk build/$(APKFILE)


.PHONY: clean
