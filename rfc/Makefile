CXX= ~/gcc-5.4/usr/local/bin/x86_64-unknown-linux-gnu-g++
CXXFLAGS += -std=c++1y
OBJS= sample-impl.o
GEN_HDRS= sample-server.hpp sample-client.hpp

%-server.hpp: %-rpc.hpp
	echo '#ifndef '$$(echo $< | tr '.-' '_') > $@
	echo '#define '$$(echo $< | tr '.-' '_') >> $@
	echo >> $@
	echo '#define GENERATE_INTERFACE' >> $@
	echo '#include "'$<'"' >> $@
	echo '#undef GENERATE_INTERFACE' >> $@
	echo '#define GENERATE_SERVER' >> $@
	echo '#include "'$<'"' >> $@
	echo '#undef GENERATE_SERVER' >> $@
	echo '#endif' >> $@

%-client.hpp: %-rpc.hpp
	echo '#ifndef '$$(echo $< | tr '.-' '_') > $@
	echo '#define '$$(echo $< | tr '.-' '_') >> $@
	echo >> $@
	echo '#define GENERATE_INTERFACE' >> $@
	echo '#include "'$<'"' >> $@
	echo '#undef GENERATE_INTERFACE' >> $@
	echo '#define GENERATE_CLIENT' >> $@
	echo '#include "'$<'"' >> $@
	echo '#undef GENERATE_CLIENT' >> $@
	echo '#endif' >> $@

sample-impl.o: sample-impl.cpp sample-server.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: all

all: $(OBJS)

.PHONY: clean

clean:
	rm -rf $(OBJS) $(GEN_HDRS)
