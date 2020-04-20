#Create a simulator object
set ns [new Simulator]
$ns color 1 Red
$ns color 2 Blue

#Open the nam trace file
set nf [open out.nam w]
$ns namtrace-all $nf

#Define a 'finish' procedure
proc finish {} {
        global ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Execute nam on the trace file
        exec nam out.nam &
        exit 0
}

#Create two nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]

#Create a duplex link between the nodes
$ns duplex-link $n0 $n1 1Mb 10ms DropTail
$ns duplex-link $n1 $n2 0.5Mb 20ms DropTail

#Create a UDP agent and attach it to node n0
set udp0 [new Agent/UDP]
$ns attach-agent $n0 $udp0
$udp0 set fid_ 1

# Create a CBR traffic source and attach it to udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 set packetSize_ 500
$cbr0 set interval_ 0.005
$cbr0 attach-agent $udp0

#Create a TCP agent (called tcp0) and attach it to node n1
//TO DO
//TO DO
$tcp0 set fid_ 2

# Create a FTP traffic source (called ftp0) and attach it to tcp0
//TO DO
//TO DO
$ftp0 set type_ FTP

#Create a Null agent (a traffic sink) and attach it to node n1
set null0 [new Agent/Null]
$ns attach-agent $n1 $null0

#Connect the udp traffic source with the traffic sink
$ns connect $udp0 $null0  

#Create a TCP traffic sink (called 'sink') and attach it to node n2
set sink [new Agent/TCPSink]
//TO DO

#Connect the tcp traffic source with the traffic sink
//TO DO

#Schedule events for the agents
$ns at 0.5 "$cbr0 start"
$ns at 1.0 "$ftp0 start"
$ns at 4.0 "$ftp0 stop"
$ns at 4.5 "$cbr0 stop"
#Call the finish procedure after 5 seconds of simulation time
$ns at 5.0 "finish"

#Run the simulation
$ns run
