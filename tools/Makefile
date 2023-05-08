CFLAGS = -Og -Wall -Wextra -Werror -ggdb -Wno-error=unused-parameter -Wno-error=unused-variable
CFLAGS += $(shell pkg-config --cflags freetype2)
LDFLAGS = $(shell pkg-config --libs freetype2)

all: ttf-convert

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

%: %.o
	$(CXX) $(LDFLAGS) $< -o $@

.SUFFIXES:

# /usr/share/fonts/OTF/ipagp.ttf
