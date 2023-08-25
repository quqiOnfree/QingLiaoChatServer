# 数据包格式
## 基本格式
基本格式是：长度 + requestID + 类型 + 序列 + 消息验证码 + 消息 + 两个\0字符
| 类型 | 名称 | 值 | 注释 |
| :---: | :---: | :---: | :---: |
| int | length | 数据包长度 |  |
| long long | requestID | 数据包请求id |  |
| int | type | 默认为0 | 数据包的类型 |
| int | sequence | 默认值：-1 | 数据包如果有分段的时候，序列就会用到 |
| size_t | verifycode | hash | 消息验证码，一般是hash验证 |
| char | data | 二进制数据 ||
| char | ___ | 两个'\0'字符 | 数据包结尾 |