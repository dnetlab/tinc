
# Forward Error Correction
## Features and Design
- [x] compitable with existing version 1.1
- [x] adding FEC enconding and deconding to net_packet.c
- [x] 4ms wait or xxx buf limit to trigger the FEC encoding and sent 
- [x] 如果延时小于10ms (过去10秒的平均值，每隔 10分钟测量一次），不使用FEC
- [x] 使用FEC时，测量丢包率（过去10秒的rolling average），发包冗余 = 20%(2 <= 丢包率 <= 10 ) 30%(10 <= 丢包率 <= 20 ) 40%(20 <= 丢包率) 

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
A -> B : net_packet:send_udp_probe_packet
B -> A : net_packet:send_udp_probe_reply
A -> B : net_packet:send_fec_probe_packet
B -> A : net_packet:send_fec_probe_reply
@enduml

```

### FEC Encoding and Decoding

把要发送的UDP数据使用FEC编码和解码

```

@startuml
A -> A : net_packet:handle_device_data
A -> A : route:route
A -> A : route:route_mac
A -> A : net_packet:send_packet
A -> A : net_packet:send_fecpacket
A -> A : net_packet:send_to_fec
A -[#blue]> A :myfec:myfec_encode_input
A -[#blue]> A :myfec:myfec_encode_output
A -> B : net_packet:sendto
B -> B : net_packet:handle_incoming_vpn_data
B -> B : net_packet:handle_incoming_vpn_packet
B -> B : net_packet:handle_incoming_vpn_packet
B -> B : net_packet:receive_fecpacket
B -[#blue]> B : myfec:myfec_decode
B -> B : net_packet:write to vnic...
@enduml

```



### FEC Feedback Control

```

@startuml
B -[#blue]> B : myfec:calculate_packet_lossy
B -> A : net_packet:send_fec_feedback
A -[#blue]> A : myfec:myfec_adjust_params
@enduml

```
### 丢包率
  丢包率计时器
    启动计时器，时间为10秒
    如果接收到100个包， 进入丢包率计算
      否则等待计时结束，计入丢包率计算
  丢包率计算
    如果接收到的包多于100个计算丢包率，重置丢包率相关参数
      否则不重置丢包率相关参数
    重启计时器

## Testing

Probe
