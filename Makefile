APPLICATION = prosoft20
RIOTBASE ?= $(CURDIR)/../RIOT/
BOARD ?= native
USEMODULE += xtimer
QUIET ?= 1

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

FEATURES_REQUIRED += periph_timer
FEATURES_REQUIRED += periph_gpio

include $(RIOTBASE)/Makefile.include
