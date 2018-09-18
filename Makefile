#
#
# Copyright (C) 2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=usb-test
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/usb-test
  SECTION:=base
  CATEGORY:=gl-inet
  TITLE:=usb-test tool
  DEPENDS:=+libusb-1.0
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

define Package/usb-test/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/usbtest  $(1)/usr/bin
endef

$(eval $(call BuildPackage,usb-test))
