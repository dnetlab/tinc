
# Forward Error Correction
## Features and Design
- [x] compitable with existing version 1.1
- [x] adding FEC enconding and deconding to net_packet.c
- [x] 4ms wait or xxx buf limit to trigger the FEC encoding and sent 
- [ ] 如果延时小于10ms (过去10秒的平均值，每隔 10分钟测量一次），不使用FEC
- [ ] 使用FEC时，测量丢包率（过去10秒的rolling average），发包冗余 = 丢包率 x 1.1 with minimal 5%, max 30% and Delta each change less than 20%

## Configuration Variables

- enable and disable

- threshhold = 10ms
- measuring_period = 10s
- change_step = 20%
- loss_mulitple = 1.1

- max_ratio = 1.382
- fec_encoding_timeout = 4ms
- fec_encoding_buf_size = 

## Key Workflows

### FEC Probe after udp tunnel established

Check if the other node support FEC.

```

@startuml
A -> B : send_udp_probe_packet
B -> A : send_udp_probe_reply
A -> B : send_fec_probe_packet
B -> A : send_fec_probe_reply
@enduml

```

### FEC Encoding and Decoding

把要发送的UDP数据使用FEC编码和解码

```

@startuml
A -> A : handle_device_data
A -> A : route
A -> A : net_packet:send_packet
A -> A : net_packet:send_fecpacket
A -> A : net_packet:send_to_fec
A -> B : sendto
@enduml

```



### FEC Feedback Control

```

@startuml
B -> B : calculate_packet_lossy
B -> A : send_fec_feedback
A -> A : myfec_adjust_params
@enduml

```

## Testing

Probe