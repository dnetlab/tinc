
##Integration Workflow
##Features
- [] 如果延时小于10ms (过去10秒的平均值，每隔 10分钟测量一次），不使用FEC
- [] 使用FEC时，测量丢包率（过去10秒的rolling average），发包冗余 = 丢包率 x 1.1 with minimal 5%, max 30% and Delta each change less than 20%

##FEC Probe after udp tunnel established

```

@startuml
A -> B : send_udp_probe_packet
B -> A : send_udp_probe_reply
A -> B : send_fec_probe_packet
B -> A : send_fec_probe_reply
@enduml

```

##FEC Sent to another side

```

@startuml
A -> A : handle_device_data
A -> A : route
A -> A : send_packet
A -> A : send_fecpacket
A -> B : sendto
@enduml

```

##FEC feedback

```

@startuml
B -> B : calculate_packet_lossy
B -> A : send_fec_feedback
A -> A : myfec_adjust_params
@enduml

```
