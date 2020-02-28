include $(TOPDIR)/rules.mk

PKG_NAME:=libspeedtest
PKG_RELEASE:=1
PKG_VERSION:=1

include $(INCLUDE_DIR)/package.mk

define Package/libspeedtest
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=libspeedtest
	DEPENDS:=+liblua +libcurl
endef

define Package/libspeedtest/description
	This is an example app package
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
	$(TARGET_CONFIGURE_OPTS)
endef

define Package/libspeedtest/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libspeedtest.so $(1)/usr/lib/lua/luci/libspeedtest.so
endef

$(eval $(call BuildPackage,libspeedtest))
