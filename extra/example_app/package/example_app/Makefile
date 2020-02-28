include $(TOPDIR)/rules.mk

PKG_NAME:=exampleapp
PKG_RELEASE:=1
PKG_VERSION:=1

include $(INCLUDE_DIR)/package.mk

define Package/exampleapp
	SECTION:=base
	CATEGORY:=Base system
	TITLE:=exampleapp
endef

define Package/exampleapp/description
	This is an example app package
endef

define Package/exampleapp/conffiles
	/etc/config/example_app
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
	$(TARGET_CONFIGURE_OPTS)
endef

define Package/exampleapp/install
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/etc/config/
	$(INSTALL_DIR) $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/exampleapp $(1)/sbin/exampleapp
	$(INSTALL_BIN) ./files/example_app.init $(1)/etc/init.d/example_app
	$(INSTALL_BIN) ./files/example_app.config $(1)/etc/config/example_app
endef

$(eval $(call BuildPackage,exampleapp))