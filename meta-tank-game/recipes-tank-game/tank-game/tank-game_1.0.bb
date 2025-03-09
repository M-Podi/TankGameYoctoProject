SUMMARY = "Tank Game for Raspberry Pi"
DESCRIPTION = "A simple tank game using ncurses for Raspberry Pi"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://tank-game.c \
           file://Makefile \
           file://tank-game.service \
          "

S = "${WORKDIR}"

DEPENDS = "ncurses"
RDEPENDS:${PN} = "ncurses ncurses-libtinfo ncurses-libncurses"

inherit systemd

SYSTEMD_SERVICE:${PN} = "tank-game.service"
SYSTEMD_AUTO_ENABLE = "enable"

TARGET_CC_ARCH += "${LDFLAGS}"

do_install() {
    oe_runmake install DESTDIR=${D}
    
    # Install systemd service file
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${WORKDIR}/tank-game.service ${D}${systemd_system_unitdir}/
    fi
}

FILES:${PN} += "${systemd_system_unitdir}/tank-game.service"