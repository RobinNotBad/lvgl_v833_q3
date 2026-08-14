/**
 * @file lv_ime_pinyin.c
 * 从8.3主线拿来的拼音输入法
 * 尝试升级lvgl版本结果出现一堆毛病，于是不升了，稳定最重要
 * 这个输入法也是毛病多多
 * 100ask是怎么在有明显数组越界问题的情况下把这玩意提交到主线里的？？？
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_ime_pinyin.h"
#if LV_USE_IME_PINYIN != 0

#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS    &lv_ime_pinyin_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_style_change_event(lv_event_t * e);
static void lv_ime_pinyin_kb_event(lv_event_t * e);
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e);

static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict);
static void pinyin_input_proc(lv_obj_t * obj);
static void pinyin_page_proc(lv_obj_t * obj, uint16_t btn);
static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num);
static void pinyin_ime_clear_data(lv_obj_t * obj);

#if LV_IME_PINYIN_USE_K9_MODE
    static void pinyin_k9_init_data(lv_obj_t * obj);
    static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[]);
    static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str);
    static void pinyin_k9_fill_cand(lv_obj_t * obj);
    static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_ime_pinyin_class = {
    .constructor_cb = lv_ime_pinyin_constructor,
    .destructor_cb  = lv_ime_pinyin_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .group_def      = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size  = sizeof(lv_ime_pinyin_t),
    .base_class     = &lv_obj_class
};

#if LV_IME_PINYIN_USE_K9_MODE
static char * lv_btnm_def_pinyin_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21] = {\
                                                                                ",\0", "1#\0",  "abc \0", "def\0",  LV_SYMBOL_BACKSPACE"\0", "\n\0",
                                                                                ".\0", "ghi\0", "jkl\0", "mno\0",  LV_SYMBOL_KEYBOARD"\0", "\n\0",
                                                                                "?\0", "pqrs\0", "tuv\0", "wxyz\0",  LV_SYMBOL_NEW_LINE"\0", "\n\0",
                                                                                LV_SYMBOL_LEFT"\0", "\0"
                                                                               };

static lv_btnmatrix_ctrl_t default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 17] = { 1 };
static char   lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 2][LV_IME_PINYIN_K9_MAX_INPUT] = {0};
#endif

static char   lv_pinyin_cand_str[LV_IME_PINYIN_CAND_TEXT_NUM][4];
static char * lv_btnm_def_pinyin_sel_map[LV_IME_PINYIN_CAND_TEXT_NUM + 3];

#if LV_IME_PINYIN_USE_DEFAULT_DICT
lv_pinyin_dict_t lv_ime_pinyin_def_dict[] = {
    {"a", "啊阿"},
    {"ai", "爱哎唉挨矮哀埃皑蔼艾碍隘癌"},
    {"an", "按安暗岸俺案鞍氨胺庵揞犴铵桉谙鹌埯黯"},
    {"ang", "昂肮盎"},
    {"ao", "凹敖熬翱袄傲奥懊澳"},
    {"ba", "吧八巴捌叭芭笆疤拔跋靶把坝霸罢爸扒耙"},
    {"bai", "白摆佰败拜柏百稗伯"},
    {"ban", "半办绊扮拌伴瓣般斑班搬扳颁板版"},
    {"bang", "邦帮梆榜绑棒镑傍谤膀磅蚌"},
    {"bao", "包保饱宝抱报豹鲍爆剥薄暴刨炮曝堡苞胞褒雹"},
    {"bei", "杯碑悲卑北辈背贝钡倍狈备惫焙被"},
    {"ben", "奔苯本笨"},
    {"beng", "崩绷甭泵蹦迸蚌"},
    {"bi", "逼鼻比鄙笔彼碧蓖蔽毕毙毖币庇痹闭敝弊必壁避陛辟臂秘"},
    {"bian", "扁便遍边编变贬辨辩辫鞭卞"},
    {"biao", "标彪膘表飙镖婊裱"},
    {"bie", "鳖憋别瘪"},
    {"bin", "彬斌濒滨宾摈"},
    {"bing", "兵冰柄丙秉饼炳病并屏"},
    {"bo", "玻菠播拨钵博勃搏铂箔帛舶脖膊渤驳柏剥薄波泊卜般伯"},
    {"bu", "捕哺补埠布步簿部怖卜不埔堡"},
    {"ca", "擦嚓"},
    {"cai", "猜裁材才财睬踩采彩菜蔡"},
    {"can", "餐残惭惨灿蚕参掺"},
    {"cang", "苍舱仓沧藏"},
    {"cao", "操糙槽曹草"},
    {"ce", "策册测厕侧"},
    {"cen", "参岑"},
    {"ceng", "层蹭曾"},
    {"cha", "插叉茶碴搽察岔诧茬查刹喳差"},
    {"chai", "柴豺拆差"},
    {"chan", "搀蝉馋谗缠铲产阐颤掺单"},
    {"chang", "昌猖场尝常偿肠厂畅唱倡长敞裳"},
    {"chao", "超抄钞潮巢吵炒朝嘲绰剿"},
    {"che", "扯撤掣彻澈车"},
    {"chen", "郴臣辰尘晨忱沉陈趁衬橙沈称秤"},
    {"cheng", "撑城成呈程惩诚承逞骋橙乘澄盛称秤"},
    {"chi", "痴持池迟弛驰耻齿侈赤翅斥炽吃匙尺"},
    {"chong", "充冲崇宠虫重"},
    {"chou", "抽酬畴踌稠愁筹仇绸瞅丑臭"},
    {"chu", "初出橱厨躇锄雏滁除楚础储矗搐触处畜"},
    {"chuai", "揣"},
    {"chuan", "川穿椽船喘串传"},
    {"chuang", "疮窗床闯创"},
    {"chui", "吹炊捶锤垂椎"},
    {"chun", "春椿醇唇淳纯蠢"},
    {"chuo", "戳绰"},
    {"ci", "疵茨磁雌辞慈瓷词此刺赐次伺兹差"},
    {"cong", "聪葱囱匆从丛"},
    {"cou", "凑"},
    {"cu", "粗醋簇促卒"},
    {"cuan", "蹿篡窜攒"},
    {"cui", "摧崔催脆瘁粹淬翠"},
    {"cun", "村存寸"},
    {"cuo", "磋搓措挫错撮"},
    {"da", "大打达搭答哒妲耷沓"},
    {"dai", "戴带待代呆袋歹贷傣殆逮怠"},
    {"dan", "耽担丹郸胆旦氮但惮淡诞蛋掸弹石单"},
    {"dang", "当挡党荡档"},
    {"dao", "刀捣蹈倒岛祷导到稻悼道盗"},
    {"de", "德得的地"},
    {"dei", "得"},
    {"deng", "蹬灯登等瞪凳邓澄"},
    {"di", "低滴迪敌笛狄涤嫡抵蒂第帝弟递缔的堤翟底地提"},
    {"dian", "颠掂滇碘点典靛垫电甸店惦奠淀殿佃"},
    {"diao", "碉叼雕凋刁掉吊钓调"},
    {"die", "跌爹碟蝶迭谍叠"},
    {"ding", "盯叮钉顶鼎锭定订丁"},
    {"diu", "丢"},
    {"dong", "东冬董懂动栋冻洞侗恫"},
    {"dou", "兜抖斗陡豆逗痘都"},
    {"du", "督毒犊独堵睹赌杜镀肚渡妒都读度"},
    {"duan", "端短锻段断缎"},
    {"dui", "兑队对堆"},
    {"dun", "墩吨钝遁蹲敦顿囤盾"},
    {"duo", "掇哆多夺垛躲朵跺剁惰度舵堕"},
    {"e", "峨鹅俄额讹娥厄扼遏鄂饿阿蛾恶哦"},
    {"en", "恩"},
    {"er", "而耳尔饵洱二贰儿"},
    {"fa", "发罚筏伐乏阀法珐"},
    {"fan", "藩帆翻樊矾钒凡烦反返范贩犯饭泛番繁"},
    {"fang", "坊芳方肪房防妨仿访纺放"},
    {"fei", "菲非啡飞肥匪诽吠肺废沸费"},
    {"fen", "芬酚吩氛分纷坟焚汾粉奋份忿愤粪"},
    {"feng", "风丰封枫蜂峰锋疯烽逢缝讽奉凤冯"},
    {"fo", "佛"},
    {"fou", "否缶"},
    {"fu", "夫敷肤孵扶辐幅氟符伏俘服浮涪福袱弗甫抚辅俯釜斧腑府腐赴副覆赋复傅付阜父腹负富讣附妇缚咐佛拂脯"},
    {"ga", "噶嘎夹咖"},
    {"gai", "该改概钙溉盖芥"},
    {"gan", "干甘杆柑竿肝赶感秆敢赣乾"},
    {"gang", "冈刚钢缸肛纲岗港杠扛"},
    {"gao", "篙皋高膏羔糕搞稿镐告"},
    {"ge", "哥歌搁戈鸽疙割葛格阁隔铬个各胳革蛤咯"},
    {"gei", "给"},
    {"gen", "根跟"},
    {"geng", "耕更庚羹埂耿梗粳颈"},
    {"gong", "工攻功恭龚供躬公宫弓巩拱贡共汞"},
    {"gou", "钩勾沟苟狗垢构购够"},
    {"gu", "辜菇咕箍估沽孤姑古蛊骨股故顾固雇鼓谷贾"},
    {"gua", "刮瓜剐寡挂褂"},
    {"guai", "乖拐怪"},
    {"guan", "棺关官冠观管馆罐惯灌贯纶"},
    {"guang", "光逛广"},
    {"gui", "瑰规圭归闺轨鬼诡癸桂柜跪贵刽硅傀炔龟"},
    {"gun", "辊滚棍"},
    {"guo", "锅郭国果裹过涡"},
    {"ha", "蛤哈"},
    {"hai", "骸孩海氦亥害骇还咳"},
    {"han", "酣憨邯韩含涵寒函喊罕翰撼捍旱憾悍焊汗汉"},
    {"hang", "杭航夯吭巷行"},
    {"hao", "壕嚎豪毫郝好耗号浩镐貉"},
    {"he", "喝荷菏禾何盒阂河赫褐鹤贺核合涸吓呵貉和"},
    {"hei", "黑嘿"},
    {"hen", "痕很狠恨"},
    {"heng", "亨横衡恒哼行"},
    {"hong", "轰哄烘虹鸿洪宏弘红"},
    {"hou", "喉侯猴吼厚候后"},
    {"hu", "呼乎忽瑚壶葫胡蝴狐糊湖弧虎护互沪户唬和"},
    {"hua", "花华猾画化话哗滑划"},
    {"huai", "槐怀淮徊坏"},
    {"huan", "欢环桓缓换患唤痪豢焕涣宦幻还"},
    {"huang", "荒慌黄磺蝗簧皇凰惶煌晃幌恍谎"},
    {"hui", "灰挥辉徽恢蛔回毁悔慧卉惠晦贿秽烩汇讳诲绘会"},
    {"hun", "昏婚魂浑混荤"},
    {"huo", "或活伙火和获惑霍货祸豁"},
    {"i", ""},
    {"ji", "击圾基机畸积箕肌饥迹激讥鸡姬绩吉极棘辑籍集及急疾汲即嫉级挤几脊己蓟技冀季伎剂悸济寄寂计记既忌际妓继纪给稽缉"
           "祭藉期奇齐系"},
    {"jia", "嘉枷佳加荚颊甲钾假稼架驾嫁夹贾价搅茄缴家"},
    {"jian", "歼监坚尖笺间煎兼肩艰奸缄茧检柬碱拣捡简俭剪减荐鉴践贱键箭件健舰剑饯渐溅涧建槛见浅"},
    {"jiang", "僵姜浆江疆蒋桨奖讲匠酱将降强"},
    {"jiao", "椒礁焦胶交郊浇骄娇脚教轿较叫窖蕉嚼搅铰狡饺绞酵觉校矫侥角缴剿"},
    {"jie", "揭接皆秸街阶截劫节杰捷睫竭洁结姐戒界借介疥诫届桔解藉芥"},
    {"jin", "巾筋斤金今津襟紧锦仅谨进靳晋禁近烬浸尽劲"},
    {"jing", "荆兢茎睛晶鲸京惊精经井警静境敬镜径痉靖竟竞净劲粳景颈"},
    {"jiong", "炯窘囧"},
    {"jiu", "就揪究纠玖韭久灸九酒厩救旧臼舅咎疚"},
    {"ju", "句居咀拘剧车桔狙菊局矩举沮聚拒据巨具距踞锯俱惧炬鞠疽驹"},
    {"juan", "捐鹃娟倦眷绢卷圈"},
    {"jue", "撅攫抉掘倔爵决诀绝嚼觉角"},
    {"jun", "菌钧军君峻俊竣郡骏均浚"},
    {"ka", "喀咖卡咔"},
    {"kai", "开揩凯慨楷"},
    {"kan", "看刊堪勘坎侃砍槛龛瞰"},
    {"kang", "康慷糠抗亢炕扛"},
    {"kao", "考拷烤靠尻铐犒"},
    {"ke", "可科坷苛柯棵磕颗渴克刻客课壳咳嗑"},
    {"ken", "肯啃垦恳"},
    {"keng", "坑吭铿"},
    {"kong", "空恐孔控箜"},
    {"kou", "抠口扣寇叩蔻"},
    {"ku", "枯哭窟苦酷库裤"},
    {"kua", "夸垮挎跨胯"},
    {"kuai", "块筷侩快会脍"},
    {"kuan", "宽款髋"},
    {"kuang", "匡筐狂框矿眶旷况哐诓"},
    {"kui", "亏盔岿窥葵奎魁馈愧傀溃"},
    {"kun", "坤昆捆困"},
    {"kuo", "扩廓阔括"},
    {"la", "垃拉喇辣啦蜡腊落"},
    {"lai", "莱来赖"},
    {"lan", "婪栏拦篮阑兰澜谰揽览懒缆烂滥蓝"},
    {"lang", "琅榔狼廊郎朗浪"},
    {"lao", "捞劳牢老佬涝姥酪烙潦落"},
    {"le", "勒乐肋了"},
    {"lei", "雷镭蕾磊累儡垒擂类泪勒肋"},
    {"leng", "楞冷棱"},
    {"li", "厘梨犁黎篱狸离漓理李里鲤礼莉荔吏栗丽厉励砾历利例俐痢立粒沥隶力璃哩"},
    {"lian", "联莲连镰廉涟帘敛脸链恋炼练怜"},
    {"liang", "粮凉梁粱良两辆量晾亮谅俩"},
    {"liao", "撩聊僚疗燎寥辽撂镣廖料潦了"},
    {"lie", "列裂烈劣猎"},
    {"lin", "琳林磷霖临邻鳞淋凛赁吝拎"},
    {"ling", "玲菱零龄铃伶羚凌灵陵岭领另令棱怜"},
    {"liu", "溜琉榴硫馏留刘瘤流柳六陆"},
    {"long", "龙聋咙笼窿隆垄拢陇弄"},
    {"lou", "楼娄搂篓漏陋露"},
    {"lu", "芦卢颅庐炉掳卤虏鲁麓路赂鹿潞禄录戮吕六碌露陆绿"},
    {"lv", "驴铝侣旅履屡缕虑氯律滤绿率"},
    {"lve", "掠略"},
    {"luan", "峦挛孪滦卵乱"},
    {"lun", "抡轮伦仑沦论纶"},
    {"luo", "萝螺罗逻锣箩骡裸洛骆烙络落咯"},
    {"ma", "妈麻玛码蚂马骂嘛吗摩抹么"},
    {"mai", "买麦卖迈埋脉"},
    {"man", "瞒馒蛮满曼慢漫谩埋蔓"},
    {"mang", "茫盲氓忙莽芒"},
    {"mao", "猫茅锚毛矛铆卯茂帽貌贸冒"},
    {"me", "么"},
    {"mei", "玫枚梅酶霉煤眉媒镁每美昧寐妹媚没糜"},
    {"men", "们门焖闷"},
    {"meng", "萌蒙檬锰猛梦孟盟"},
    {"mi", "眯醚靡迷弥米觅蜜密幂糜谜泌秘"},
    {"mian", "棉眠绵冕免勉缅面娩"},
    {"miao", "喵苗描瞄藐秒渺庙妙"},
    {"mie", "蔑灭"},
    {"min", "民抿皿敏悯闽"},
    {"ming", "明螟鸣铭名命"},
    {"miu", "谬"},
    {"mo", "摸摹蘑膜磨魔末莫墨默沫漠寞陌脉没模摩抹"},
    {"mou", "谋某牟"},
    {"mu", "拇牡亩姆母墓暮幕募慕木目睦牧穆姥模牟"},
    {"na", "拿钠纳呐那娜哪"},
    {"nai", "氖乃奶耐奈哪"},
    {"nan", "南男难"},
    {"nang", "囊"},
    {"nao", "挠脑恼闹淖"},
    {"ne", "呢哪"},
    {"nei", "馁内那哪"},
    {"nen", "嫩恁"},
    {"neng", "能"},
    {"ni", "妮霓倪泥尼拟你匿腻逆溺呢"},
    {"nian", "蔫拈年碾撵捻念粘"},
    {"niang", "娘酿"},
    {"niao", "鸟尿"},
    {"nie", "捏聂孽啮镊镍涅"},
    {"nin", "您"},
    {"ning", "柠狞凝宁拧泞"},
    {"niu", "牛扭钮纽"},
    {"nong", "脓浓农弄"},
    {"nu", "奴怒努"},
    {"nv", "女"},
    {"nve", "虐疟"},
    {"nuan", "暖"},
    {"nuo", "挪懦糯诺娜"},
    {"o", "哦喔噢"},
    {"ou", "欧鸥殴藕呕偶沤区"},
    {"pa", "啪趴爬帕怕扒耙琶"},
    {"pai", "拍排牌徘湃派迫"},
    {"pan", "攀潘盘磐盼畔判叛番胖般"},
    {"pang", "乓庞耪膀磅旁胖"},
    {"pao", "抛咆袍跑泡刨炮"},
    {"pei", "呸胚培裴赔陪配佩沛坏"},
    {"pen", "喷盆"},
    {"peng", "砰抨烹澎彭蓬棚硼篷膨朋鹏捧碰"},
    {"pi", "坯砒霹批披劈琵毗啤脾疲皮痞僻屁譬辟否匹坏"},
    {"pian", "篇偏片骗扁便"},
    {"piao", "飘漂瓢票朴"},
    {"pie", "撇瞥"},
    {"pin", "拼频贫品聘"},
    {"ping", "乒坪萍平凭瓶评苹屏"},
    {"po", "坡泼颇婆破粕泊迫魄朴"},
    {"pou", "剖"},
    {"pu", "扑铺仆莆葡菩蒲圃普浦谱脯埔曝瀑堡朴"},
    {"qi", "欺戚妻七凄柒沏棋歧崎脐旗祈祁骑起岂乞企启器气迄弃汽讫稽缉期栖其奇畦齐砌泣漆契"},
    {"qia", "掐卡洽"},
    {"qian", "牵扦钎千迁签仟谦黔钱钳前潜遣谴堑欠歉铅乾浅嵌纤"},
    {"qiang", "枪呛腔羌墙蔷抢强"},
    {"qiao", "锹敲悄桥乔侨巧撬翘峭俏窍壳橇瞧鞘雀"},
    {"qie", "切窃砌茄且怯"},
    {"qin", "钦侵秦琴勤芹擒禽寝亲沁"},
    {"qing", "青轻氢倾卿清擎晴氰情顷请庆亲"},
    {"qiong", "琼穷"},
    {"qiu", "秋丘邱球求囚酋泅"},
    {"qu", "趋曲躯屈驱渠取娶龋去区蛆趣"},
    {"quan", "颧权醛泉全痊拳犬券劝卷圈"},
    {"que", "缺瘸却鹊榷确炔雀"},
    {"qun", "裙群"},
    {"ran", "然燃冉染"},
    {"rang", "瓤壤攘嚷让"},
    {"rao", "饶扰绕"},
    {"re", "惹热"},
    {"ren", "壬仁人忍韧任认刃妊纫"},
    {"reng", "扔仍"},
    {"ri", "日"},
    {"rong", "戎茸蓉荣融熔溶容绒冗"},
    {"rou", "揉柔肉"},
    {"ru", "茹儒孺如辱乳汝入褥蠕"},
    {"ruan", "软阮"},
    {"rui", "蕊瑞锐"},
    {"run", "闰润"},
    {"ruo", "弱若"},
    {"sa", "撒洒萨"},
    {"sai", "腮鳃赛塞"},
    {"san", "三叁伞散"},
    {"sang", "桑嗓丧"},
    {"sao", "搔骚扫嫂梢"},
    {"se", "瑟涩塞色"},
    {"sen", "森"},
    {"seng", "僧"},
    {"sha", "砂杀沙纱傻啥煞莎刹杉厦"},
    {"shai", "筛晒色"},
    {"shan", "山删煽衫闪陕擅赡膳善汕扇缮杉栅掺单珊苫"},
    {"shang", "伤商赏晌上尚裳汤墒殇觞熵"},
    {"shao", "捎稍烧芍勺韶少哨邵绍鞘梢召"},
    {"she", "奢赊舌舍赦摄慑涉社设蛇拾折射"},
    {"shei", "谁"},
    {"shen", "砷申呻伸身深绅神审婶肾慎渗沈甚参娠什"},
    {"sheng", "声生甥牲升绳剩胜圣乘省盛"},
    {"shi", "是十失事世始式示师狮施湿诗尸虱时蚀实史矢使屎驶士柿拭誓逝势嗜噬适仕侍释饰市恃室视试匙石拾食识氏似嘘殖峙什"},
    {"shou", "收手首守寿授售受瘦兽熟"},
    {"shu", "蔬枢梳殊抒输叔舒淑疏书赎孰薯暑曙署蜀黍鼠述树束戍竖墅庶漱恕熟属术数"},
    {"shua", "刷耍"},
    {"shuai", "摔甩帅衰率"},
    {"shuan", "栓拴"},
    {"shuang", "霜双爽"},
    {"shui", "水睡税谁说"},
    {"shun", "吮瞬顺舜"},
    {"shuo", "硕朔烁数说"},
    {"si", "斯撕嘶私司丝死肆寺嗣四饲巳食思伺似"},
    {"song", "松耸怂颂送宋讼诵"},
    {"sou", "搜擞嗽艘"},
    {"su", "苏酥俗素速粟僳塑溯诉肃宿缩"},
    {"suan", "酸蒜算"},
    {"sui", "虽隋随绥髓碎岁穗遂隧祟尿"},
    {"sun", "孙损笋"},
    {"suo", "蓑梭唆琐索锁所莎缩"},
    {"ta", "塌他它她獭挞蹋踏塔拓"},
    {"tai", "胎苔抬台泰酞太态汰"},
    {"tan", "坍摊贪瘫滩坛檀痰潭谭谈坦毯袒碳探叹炭弹"},
    {"tang", "塘搪堂棠膛唐糖躺淌趟烫敞汤倘"},
    {"tao", "掏涛滔绦萄桃逃淘讨套陶"},
    {"te", "特"},
    {"teng", "藤腾疼誊"},
    {"ti", "梯剔踢锑题蹄啼体替嚏惕涕剃屉提"},
    {"tian", "天添填田甜恬舔腆蚕"},
    {"tiao", "挑条迢眺跳调"},
    {"tie", "贴铁帖"},
    {"ting", "厅烃汀廷停亭庭挺艇听"},
    {"tong", "通桐酮瞳同铜彤童桶捅筒统痛侗恫"},
    {"tou", "偷投头透"},
    {"tu", "秃突图徒途涂屠土吐兔凸余"},
    {"tuan", "湍团"},
    {"tui", "推颓腿蜕退褪"},
    {"tun", "吞屯臀囤"},
    {"tuo", "拖托脱鸵陀驼椭妥唾驮拓"},
    {"u", ""},
    {"v", ""},
    {"wa", "挖哇蛙洼娃瓦袜"},
    {"wai", "歪外"},
    {"wan", "豌弯湾玩顽丸烷完碗挽晚惋婉腕蔓皖宛万"},
    {"wang", "汪王枉网往旺望忘妄亡"},
    {"wei", "威巍微危韦违桅围唯惟为潍维苇萎委伟伪纬未味畏胃喂魏位渭谓慰卫尾蔚尉"},
    {"wen", "瘟温蚊文闻纹吻稳紊问"},
    {"weng", "嗡翁瓮"},
    {"wo", "挝蜗窝我斡卧握沃涡"},
    {"wu", "巫呜钨乌污诬屋芜梧吾吴毋武五捂午舞伍侮坞戊雾晤物勿务悟误恶无"},
    {"xi", "昔熙析西硒矽晰嘻吸锡牺稀息希悉膝夕惜熄烯汐犀檄袭席习媳喜隙细栖溪铣洗系戏"},
    {"xia", "瞎匣霞辖暇峡侠狭下虾厦夏吓"},
    {"xian", "掀锨先仙鲜咸贤衔舷闲涎弦嫌显险现献县腺馅羡宪陷限线铣纤"},
    {"xiang", "相厢镶香箱襄湘乡翔祥详想响享项橡像向象降巷"},
    {"xiao", "萧硝霄哮销消宵晓小孝肖啸笑效削嚣淆校"},
    {"xie", "楔些歇鞋协携胁谐写械卸蟹懈泄泻谢屑解蝎挟邪斜血叶契"},
    {"xin", "薪芯锌欣辛新忻心衅信"},
    {"xing", "星腥猩惺兴刑型形邢醒幸杏性姓省行"},
    {"xiong", "兄凶胸匈汹雄熊"},
    {"xiu", "休修羞朽嗅锈秀袖绣臭宿"},
    {"xu", "墟需虚须徐许蓄酗叙旭序恤絮婿绪续戌嘘畜吁"},
    {"xuan", "轩喧宣悬旋玄选癣眩绚"},
    {"xue", "靴薛学穴雪削血"},
    {"xun", "勋熏循旬询驯巡殉汛训讯逊迅浚寻"},
    {"ya", "压押鸦鸭呀丫牙蚜衙涯雅哑亚讶芽崖轧"},
    {"yan", "焉阉淹盐严研蜒岩延言颜阎炎沿奄掩眼衍演艳堰燕厌砚雁唁彦焰宴谚验铅咽烟殷"},
    {"yang", "殃央鸯秧杨扬佯疡羊洋阳氧仰痒养样漾"},
    {"yao", "邀腰妖瑶摇尧遥窑谣姚咬舀药要耀约钥侥"},
    {"ye", "椰噎耶爷野冶也页业夜咽掖叶腋液拽曳"},
    {"yi",
     "一壹医揖铱依伊衣颐夷移仪胰疑沂宜姨彝椅蚁倚已乙矣以艺抑易邑亿役臆逸肄疫亦裔意毅忆义益溢诣议谊译异翼翌绎遗屹"},
    {"yin", "茵荫因音阴姻吟银淫寅饮尹引隐印殷"},
    {"ying", "英樱婴鹰应缨莹萤营荧蝇迎赢盈影颖硬映"},
    {"yo", "哟"},
    {"yong", "拥佣臃痈庸雍踊蛹咏泳永恿勇用涌"},
    {"you", "幽优悠忧尤由邮铀犹油游酉有友右佑釉诱又幼"},
    {"yu", "迂淤于盂榆虞愚舆逾鱼愉渝渔隅予娱雨与屿禹宇语羽玉域芋郁遇喻峪御愈欲狱誉浴寓裕预豫驭尉余俞吁育"},
    {"yuan", "鸳渊冤元垣袁原援辕园圆猿源缘远苑愿怨院员"},
    {"yue", "曰越跃岳粤月悦阅乐约钥"},
    {"yun", "耘云郧匀陨允运蕴酝晕韵孕均员"},
    {"za", "匝砸杂扎咱咋"},
    {"zai", "栽哉灾宰载再在仔"},
    {"zan", "暂赞攒咱"},
    {"zang", "赃脏葬藏"},
    {"zao", "遭糟藻枣早澡蚤躁噪造皂灶燥凿"},
    {"ze", "责则泽择侧咋"},
    {"zei", "贼"},
    {"zen", "怎"},
    {"zeng", "增憎赠曾综"},
    {"zha", "渣札铡闸眨榨乍炸诈查扎喳栅柞轧咋"},
    {"zhai", "斋债寨翟祭择摘宅窄侧"},
    {"zhan", "瞻毡詹沾盏斩辗崭展蘸栈占战站湛绽颤粘"},
    {"zhang", "樟章彰漳张掌涨杖丈帐账仗胀瘴障长"},
    {"zhao", "招昭找沼赵照罩兆肇朝召爪着"},
    {"zhe", "遮哲蛰辙者蔗浙折锗这着"},
    {"zhen", "珍斟真甄砧臻贞针侦枕疹诊震振镇阵帧"},
    {"zheng", "蒸挣睁征狰争怔整拯正政症郑证"},
    {"zhi", "芝支蜘知肢脂汁之织职直植执值侄址指止趾只旨纸志挚掷至致置帜制智秩稚质炙痔滞治窒识枝吱殖峙"},
    {"zhong", "中盅忠钟衷终肿仲众种重"},
    {"zhou", "舟周州洲诌轴肘帚咒皱宙昼骤粥"},
    {"zhu", "珠株蛛朱猪诸诛逐竹烛煮拄瞩嘱主柱助蛀贮铸筑住注祝驻属著"},
    {"zhua", "抓爪"},
    {"zhuai", "拽"},
    {"zhuan", "专砖撰篆传转赚"},
    {"zhuang", "桩庄装妆壮状幢撞"},
    {"zhui", "锥追赘坠缀椎"},
    {"zhun", "准谆"},
    {"zhuo", "捉拙卓桌茁酌啄灼浊琢缴着"},
    {"zi", "咨资姿滋淄孜紫籽滓子自渍字吱兹仔"},
    {"zong", "鬃棕踪宗总纵综"},
    {"zou", "邹走奏揍"},
    {"zu", "阻组足卒租族祖诅"},
    {"zuan", "钻纂"},
    {"zui", "嘴醉最罪"},
    {"zun", "尊遵"},
    {"zuo", "昨左佐做作坐座撮琢柞"},
    {NULL, NULL}
};
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * lv_ime_pinyin_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the keyboard of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @param dict pointer to a Pinyin input method keyboard
 */
void lv_ime_pinyin_set_keyboard(lv_obj_t * obj, lv_obj_t * kb)
{
    if(kb) {
        LV_ASSERT_OBJ(kb, &lv_keyboard_class);
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime->kb = kb;
    lv_obj_add_event_cb(pinyin_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_align_to(pinyin_ime->cand_panel, pinyin_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @param dict pointer to a Pinyin input method dictionary
 */
void lv_ime_pinyin_set_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    init_pinyin_dict(obj, dict);
}

/**
 * Set mode, 26-key input(k26) or 9-key input(k9).
 * @param obj  pointer to a Pinyin input method object
 * @param mode   the mode from 'lv_keyboard_mode_t'
 */
void lv_ime_pinyin_set_mode(lv_obj_t * obj, lv_ime_pinyin_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    LV_ASSERT_OBJ(pinyin_ime->kb, &lv_keyboard_class);

    pinyin_ime->mode = mode;

#if LV_IME_PINYIN_USE_K9_MODE
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_k9_init_data(obj);
        lv_keyboard_set_map(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1, (const char **)lv_btnm_def_pinyin_k9_map,
                            (const lv_btnmatrix_ctrl_t *)default_kb_ctrl_k9_map);
        lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1);
    }
#endif
}

/*=====================
 * Getter functions
 *====================*/

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin IME object
 * @return     pointer to the Pinyin IME keyboard
 */
lv_obj_t * lv_ime_pinyin_get_kb(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->kb;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method candidate panel
 */
lv_obj_t * lv_ime_pinyin_get_cand_panel(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->cand_panel;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method dictionary
 */
lv_pinyin_dict_t * lv_ime_pinyin_get_dict(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->dict;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 0; btnm_i < (LV_IME_PINYIN_CAND_TEXT_NUM + 3); btnm_i++) {
        if(btnm_i == 0) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "<";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = ">";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 2)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "";
        }
        else {
            lv_pinyin_cand_str[py_str_i][0] = ' ';
            lv_btnm_def_pinyin_sel_map[btnm_i] = lv_pinyin_cand_str[py_str_i];
            py_str_i++;
        }
    }

    pinyin_ime->mode = LV_IME_PINYIN_MODE_K26;
    pinyin_ime->py_page = 0;
    pinyin_ime->ta_count = 0;
    pinyin_ime->cand_num = 0;
    lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
    lv_memset_00(pinyin_ime->py_num, sizeof(pinyin_ime->py_num));
    lv_memset_00(pinyin_ime->py_pos, sizeof(pinyin_ime->py_pos));

    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(55));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);

#if LV_IME_PINYIN_USE_DEFAULT_DICT
    init_pinyin_dict(obj, lv_ime_pinyin_def_dict);
#endif

    /* Init pinyin_ime->cand_panel */
    
    pinyin_ime->cand_panel = lv_btnmatrix_create(obj);
    pinyin_ime->kb = lv_keyboard_create(obj);
    
    lv_obj_set_size(pinyin_ime->kb, LV_PCT(100), LV_PCT(85));
    lv_obj_set_size(pinyin_ime->cand_panel, LV_PCT(100), LV_PCT(15));

    lv_obj_align_to(pinyin_ime->cand_panel, pinyin_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_btnmatrix_set_map(pinyin_ime->cand_panel, (const char **)lv_btnm_def_pinyin_sel_map);
    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);

    lv_btnmatrix_set_one_checked(pinyin_ime->cand_panel, true);
    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Set cand_panel style*/
    // Default style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, 0);
    lv_obj_set_style_border_width(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_all(pinyin_ime->cand_panel, 8, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_base_dir(pinyin_ime->cand_panel, LV_BASE_DIR_LTR, 0);

    // LV_PART_ITEMS style
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 12, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);

    // LV_PART_ITEMS | LV_STATE_PRESSED style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);

    /* event handler */
    lv_obj_add_event_cb(pinyin_ime->cand_panel, lv_ime_pinyin_cand_panel_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_add_event_cb(obj, lv_ime_pinyin_style_change_event, LV_EVENT_STYLE_CHANGED, NULL);
    lv_obj_add_event_cb(pinyin_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);

#if LV_IME_PINYIN_USE_K9_MODE
    pinyin_ime->k9_input_str_len = 0;
    pinyin_ime->k9_py_ll_pos = 0;
    pinyin_ime->k9_legal_py_count = 0;
    lv_memset_00(pinyin_ime->k9_input_str, LV_IME_PINYIN_K9_MAX_INPUT);

    pinyin_k9_init_data(obj);

    _lv_ll_init(&(pinyin_ime->k9_legal_py_ll), sizeof(ime_pinyin_k9_py_str_t));
#endif
}

static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    /*
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    if(lv_obj_is_valid(pinyin_ime->kb))
        lv_obj_del(pinyin_ime->kb);

    if(lv_obj_is_valid(pinyin_ime->cand_panel))
        lv_obj_del(pinyin_ime->cand_panel);
    */
}

/**
 * 键盘按键事件回调（输入法核心入口）。
 * 用户在软键盘上每按下一个按键都会触发一次 LV_EVENT_VALUE_CHANGED，
 * 本函数根据按键文本 txt 分发到不同的处理分支：
 *   - 回车 / 换行：清空输入数据
 *   - 退格：删除输入缓冲区最后一个字符并重新检索候选字
 *   - 字母键（K26 全键盘）：追加到拼音串并触发候选字检索
 *   - 字母键（K9 九宫格）：按键位映射到 2~9 数字编码，再枚举合法拼音
 *   - 功能键（切换模式 / 确认 / 左右翻页等）
 */
static void lv_ime_pinyin_kb_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);         /* 触发事件的键盘对象 */
    lv_obj_t * obj = lv_event_get_user_data(e);     /* 绑定时传入的输入法对象 */

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    /* K9 九宫格按键编号(数字键 2~9)到对应字母集合的映射表 */
    static const char * k9_py_map[8] = {"abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"};
#endif

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint16_t btn_id  = lv_btnmatrix_get_selected_btn(kb);   /* 被按下按键的索引 */
        if(btn_id == LV_BTNMATRIX_BTN_NONE) return;             /* 无按键被按下则直接返回 */

        const char * txt = lv_btnmatrix_get_btn_text(kb, lv_btnmatrix_get_selected_btn(kb));
        if(txt == NULL) return;                                  /* 按键文本为空则直接返回 */

#if LV_IME_PINYIN_USE_K9_MODE
        if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
            lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
            uint16_t tmp_btn_str_len = strlen(pinyin_ime->input_char);
            if((btn_id >= 16) && (tmp_btn_str_len > 0) && (btn_id < (16 + LV_IME_PINYIN_K9_CAND_TEXT_NUM))) {
                tmp_btn_str_len = strlen(pinyin_ime->input_char);
                lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
                strcat(pinyin_ime->input_char, txt);
                pinyin_input_proc(obj);

                for(int index = 0; index < (pinyin_ime->ta_count + tmp_btn_str_len); index++) {
                    lv_textarea_del_char(ta);
                }

                pinyin_ime->ta_count = tmp_btn_str_len;
                pinyin_ime->k9_input_str_len = tmp_btn_str_len;
                lv_textarea_add_text(ta, pinyin_ime->input_char);

                return;
            }
        }
#endif

        /* 回车键（英文 "Enter" 或换行符号）: 结束本次输入，清空输入法内部数据 */
        if(strcmp(txt, "Enter") == 0 || strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
            pinyin_ime_clear_data(obj);
            lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
        }
        /* 退格键: 删除输入缓冲区中的最后一个字符 */
        else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            if(pinyin_ime->ta_count > 0) {
                /* K26 模式删除拼音串末尾字符；K9 模式删除数字编码串末尾字符 */
                if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26)
                    pinyin_ime->input_char[pinyin_ime->ta_count - 1] = '\0';
#if LV_IME_PINYIN_USE_K9_MODE
                else
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count - 1] = '\0';
#endif

                pinyin_ime->ta_count = pinyin_ime->ta_count - 1;
                /* 输入缓冲区已清空: 隐藏候选面板，重置候选区 */
                if(pinyin_ime->ta_count <= 0) {
                    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
#if LV_IME_PINYIN_USE_K9_MODE
                    lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
                    strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
                    strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
#endif
                }
                /* K26 模式: 还有剩余拼音，重新检索候选字 */
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                    pinyin_input_proc(obj);
                }
#if LV_IME_PINYIN_USE_K9_MODE
                /* K9 模式: 重新枚举剩余数字编码对应的合法拼音并刷新候选 */
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
                    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char) - 1;
                    pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
                    pinyin_k9_fill_cand(obj);
                    pinyin_input_proc(obj);
                }
#endif
            }
        }
        /* 大小写切换键 / K9 数字键 "1#": 仅清空当前拼音输入，不处理候选 */
        else if((strcmp(txt, "ABC") == 0) || (strcmp(txt, "abc") == 0) || (strcmp(txt, "1#") == 0)) {
            pinyin_ime->ta_count = 0;
            lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
            return;
        }
        /* 键盘切换键: 在 K26(全键盘) 与 K9(九宫格) 之间切换 */
        else if(strcmp(txt, LV_SYMBOL_KEYBOARD) == 0) {
            if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K9);
            }
            else {
                lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K26);
                lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_TEXT_LOWER);
            }
            pinyin_ime_clear_data(obj);
        }
        /* 确认键: 结束输入，清空数据 */
        else if(strcmp(txt, LV_SYMBOL_OK) == 0) {
            pinyin_ime_clear_data(obj);
        }
        /* K26 模式下的字母键: 把字母追加到拼音串末尾，然后检索候选字 */
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) && ((txt[0] >= 'a' && txt[0] <= 'z') || (txt[0] >= 'A' &&
                                                                                                      txt[0] <= 'Z'))) {
            strcat(pinyin_ime->input_char, txt);
            pinyin_input_proc(obj);
            pinyin_ime->ta_count++;
        }
#if LV_IME_PINYIN_USE_K9_MODE
        /* K9 模式下的数字键(字母组)输入: 按键映射为数字编码 2~9 存入 k9_input_str */
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) && (txt[0] >= 'a' && txt[0] <= 'z')) {
            for(uint16_t i = 0; i < 8; i++) {
                if((strcmp(txt, k9_py_map[i]) == 0) || (strcmp(txt, "abc ") == 0)) {
                    /* "abc " 带空格表示作为独立的拼音分隔；否则累加当前键的字母数 */
                    if(strcmp(txt, "abc ") == 0)    pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]) + 1;
                    else                            pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]);
                    /* '2' 的 ASCII 码为 50，故 50 + i 得到按键编号 2~9 */
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count] = 50 + i;

                    break;
                }
            }
            pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
            pinyin_k9_fill_cand(obj);
            pinyin_input_proc(obj);
        }
        /* K9 模式左右翻页键: dir=0 上一页, dir=1 下一页 */
        else if(strcmp(txt, LV_SYMBOL_LEFT) == 0) {
            pinyin_k9_cand_page_proc(obj, 0);
        }
        else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            pinyin_k9_cand_page_proc(obj, 1);
        }
#endif
    }
}

/**
 * 候选面板按键事件回调。
 * 候选面板是一个按钮矩阵，首尾两个按钮分别是 "<"(上一页) 和 ">"(下一页)，
 * 中间的按钮是候选汉字。用户点击候选汉字后，用该字替换 textarea 中的拼音串。
 */
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cand_panel = lv_event_get_target(e);   /* 候选面板对象 */
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(cand_panel);
        /* 点击 "<": 翻到上一页候选 */
        if(id == 0) {
            pinyin_page_proc(obj, 0);
            return;
        }
        /* 点击 ">": 翻到下一页候选 */
        if(id == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            pinyin_page_proc(obj, 1);
            return;
        }

        /* 点击了某个候选汉字: 把拼音串从 textarea 中删除，替换为选中的汉字 */
        const char * txt = lv_btnmatrix_get_btn_text(cand_panel, id);
        lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
        uint16_t index = 0;
        for(index = 0; index < pinyin_ime->ta_count; index++)
            lv_textarea_del_char(ta);   /* 逐个删除当前已输入的拼音字母 */

        lv_textarea_add_text(ta, txt);   /* 把选中的汉字写入 textarea */

        pinyin_ime_clear_data(obj);      /* 一次选择完成，清空输入法内部数据 */
    }
}

/**
 * 拼音输入处理（K26 全键盘模式的核心检索逻辑）。
 * 1. 在字典中查找以当前 input_char 为前缀的条目；
 * 2. 若找到，则把该条目的候选汉字字符串按每 3 字节(一个 UTF-8 汉字)拆分，
 *    填入候选面板的按钮数组，并显示候选面板。
 */
static void pinyin_input_proc(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    /* 检索字典：cand_str 指向匹配条目的汉字串，cand_num 为该串中的汉字个数 */
    pinyin_ime->cand_str = pinyin_search_matching(obj, pinyin_ime->input_char, &pinyin_ime->cand_num);
    if(pinyin_ime->cand_str == NULL) {
        return;   /* 未找到匹配，保持候选面板不变 */
    }

    pinyin_ime->py_page = 0;   /* 每次重新检索后回到第一页 */

    /* 清空候选按钮的显示缓冲，并为每个候选位置预填一个空格占位 */
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    /* 把匹配到的汉字按顺序填入候选按钮(每个汉字 3 字节，最多填满一屏) */
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[i * 3 + j];
        }
    }

    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);   /* 显示候选面板 */
}

/**
 * 候选字翻页处理。
 * @param dir 0 = 上一页，1 = 下一页
 * 计算总页数(page_num)和最后一页剩余的候选数(sur)，
 * 更新 py_page 后按当前页偏移重新填充候选按钮。
 */
static void pinyin_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t page_num = pinyin_ime->cand_num / LV_IME_PINYIN_CAND_TEXT_NUM;   /* 完整页数 */
    uint16_t sur = pinyin_ime->cand_num % LV_IME_PINYIN_CAND_TEXT_NUM;        /* 最后一页剩余候选数 */

    if(dir == 0) {
        /* 上一页：页号减一(下限为 0) */
        if(pinyin_ime->py_page) {
            pinyin_ime->py_page--;
        }
    }
    else {
        /* 下一页：先修正总页数(无余数时减一)，再判断是否还能翻页 */
        if(sur == 0) {
            page_num -= 1;
        }
        if(pinyin_ime->py_page < page_num) {
            pinyin_ime->py_page++;
        }
        else return;   /* 已是最后一页，无法继续翻页 */
    }

    /* 清空候选显示缓冲 */
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    /* 按当前页偏移重新填充候选汉字 */
    uint16_t offset = pinyin_ime->py_page * (3 * LV_IME_PINYIN_CAND_TEXT_NUM);
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        /* 最后一页且有余数时，只填充到 sur 为止，避免越界读 */
        if((sur > 0) && (pinyin_ime->py_page == page_num)) {
            if(i > sur)
                break;
        }
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[offset + (i * 3) + j];
        }
    }
}

static void lv_ime_pinyin_style_change_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_obj_set_style_text_font(pinyin_ime->cand_panel, font, 0);
    }
}

/**
 * 初始化字典并建立首字母索引。
 * 遍历字典(以 NULL 条目结尾)，按首字母 a~z 分组统计：
 *   - py_num[x]  记录首字母为 ('a'+x) 的条目数量
 *   - py_pos[x]  记录首字母为 ('a'+x) 的第一个条目在字典中的偏移
 * 后续 pinyin_search_matching 通过这些索引快速定位到对应首字母的区段，
 * 从而避免每次都从头线性扫描整个字典。
 */
static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    char headletter = 'a';          /* 当前正在统计的首字母 */
    uint16_t offset_sum = 0;        /* 已统计条目的累计总数，用于计算区段起始偏移 */
    uint16_t offset_count = 0;      /* 当前首字母区段内的条目计数 */
    uint16_t letter_calc = 0;       /* 首字母映射到数组下标的临时变量 */

    pinyin_ime->dict = dict;

    for(uint16_t i = 0; ; i++) {
        /* 遇到结尾哨兵条目(NULL)，记录最后一个字母区段的条目数并结束 */
        if((NULL == (dict[i].py)) || (NULL == (dict[i].py_mb))) {
            headletter = dict[i - 1].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc] = offset_count;
            break;
        }

        /* 当前条目首字母与上一分组相同，计数加一 */
        if(headletter == (dict[i].py[0])) {
            offset_count++;
        }
        else {
            /* 遇到新的首字母：结算上一分组，并记录新分组的起始偏移 */
            headletter = dict[i].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc - 1] = offset_count;   /* 上一字母区段的条目数 */
            offset_sum += offset_count;
            pinyin_ime->py_pos[letter_calc] = offset_sum;          /* 当前字母区段的起始偏移 */

            offset_count = 1;   /* 当前条目计入新分组 */
        }
    }
}

/**
 * 在字典中查找以 py_str 为前缀的拼音条目。
 * 利用 init_pinyin_dict 预先建立的首字母索引(py_num/py_pos)快速定位
 * 到 py_str 首字母对应的字典区段，然后线性扫描该区段做前缀匹配。
 *
 * @param obj     输入法对象
 * @param py_str  待匹配的拼音串(如 "zhong")
 * @param cand_num 输出参数，返回匹配条目所包含的汉字个数
 * @return        匹配条目的汉字字符串指针，未找到返回 NULL
 */
static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ;
    uint8_t index, len = 0, offset;
    volatile uint8_t count = 0;

    /* 空串以及 i/u/v 开头的串无对应拼音，直接返回 */
    if(*py_str == '\0')    return NULL;
    if(*py_str == 'i')     return NULL;
    if(*py_str == 'u')     return NULL;
    if(*py_str == 'v')     return NULL;

    /* 用首字母定位到字典区段：offset = 首字母 - 'a' */
    offset = py_str[0] - 'a';
    len = strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];   /* 该首字母区段的起始条目 */
    count = pinyin_ime->py_num[offset];                      /* 该首字母区段的条目数量 */

    while(count--) {
        /* 逐字符比较 py_str 与当前条目的拼音，做前缀匹配 */
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;   /* 出现不匹配字符，提前退出 */
            }
        }

        /* 完全匹配(单字母输入或逐字符全部匹配成功) */
        if(len == 1 || index == len) {
            /* UTF-8 编码下每个汉字占 3 字节，因此汉字个数 = 字符串字节长度 / 3 */
            * cand_num = strlen((const char *)(cpHZ->py_mb)) / 3;
            return (char *)(cpHZ->py_mb);
        }
        cpHZ++;   /* 移动到下一个字典条目 */
    }
    return NULL;
}

/**
 * 清空输入法内部状态：重置计数、清空输入缓冲与候选缓冲，并隐藏候选面板。
 * 通常在完成一次选字、回车、确认或切换键盘模式时调用。
 */
static void pinyin_ime_clear_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    /* K9 模式额外需要清理九宫格编码串与合法拼音链表相关状态 */
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_ime->k9_input_str_len = 0;
        pinyin_ime->k9_py_ll_pos = 0;
        pinyin_ime->k9_legal_py_count = 0;
        lv_memset_00(pinyin_ime->k9_input_str,  LV_IME_PINYIN_K9_MAX_INPUT);
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
    }
#endif

    pinyin_ime->ta_count = 0;
    lv_memset_00(lv_pinyin_cand_str, (sizeof(lv_pinyin_cand_str)));
    lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));

    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
}

#if LV_IME_PINYIN_USE_K9_MODE
static void pinyin_k9_init_data(lv_obj_t * obj)
{
    //lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 19; btnm_i < (LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21); btnm_i++) {
        if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM) {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], LV_SYMBOL_RIGHT"\0");
        }
        else if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1) {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], "\0");
        }
        else {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], " \0");
        }

        lv_btnm_def_pinyin_k9_map[btnm_i] = lv_pinyin_k9_cand_str[py_str_i];
        py_str_i++;
    }

    default_kb_ctrl_k9_map[0]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[4]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[5]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[9]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[10] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[14] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[15] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 16] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
}

/**
 * K9 九宫格模式：根据用户按下的数字键序列(k9_input)，枚举所有可能的拼音组合。
 * 每个数字键(2~9)对应 3~4 个字母(py9_map)，使用回溯法生成所有字母组合，
 * 并调用 pinyin_k9_is_valid_py 过滤出字典中真实存在的合法拼音，
 * 合法的拼音串通过链表 k9_legal_py_ll 保存，供候选填充使用。
 */
static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[])
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t len = strlen(k9_input);

    if((len == 0) || (len >= LV_IME_PINYIN_K9_MAX_INPUT)) {
        return;   /* 空输入或超过最大长度，直接返回 */
    }

    char py_comp[LV_IME_PINYIN_K9_MAX_INPUT] = {0};   /* 当前正在组合的拼音缓冲区 */
    int mark[LV_IME_PINYIN_K9_MAX_INPUT] = {0};       /* 记录每一位数字键已尝试到第几个字母 */
    int index = 0;                                     /* 当前正在填写的位数 */
    int flag = 0;
    int count = 0;                                     /* 已找到的合法拼音个数 */

    uint32_t ll_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    ll_len = _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);
    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);

    /* 回溯法枚举：index 表示当前处理到第几位，-1 表示回溯结束 */
    while(index != -1) {
        if(index == len) {
            /* 已填满所有位，得到一个完整拼音组合，校验其是否为合法拼音 */
            if(pinyin_k9_is_valid_py(obj, py_comp)) {
                if((count >= ll_len) || (ll_len == 0)) {
                    /* 链表容量不足则在尾部新建节点 */
                    ll_index = _lv_ll_ins_tail(&pinyin_ime->k9_legal_py_ll);
                    strcpy(ll_index->py_str, py_comp);
                }
                else if((count < ll_len)) {
                    /* 复用已有节点，覆盖旧的拼音串 */
                    strcpy(ll_index->py_str, py_comp);
                    ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                }
                count++;
            }
            index--;   /* 回溯到上一位 */
        }
        else {
            flag = mark[index];
            if(flag < strlen(py9_map[k9_input[index] - '2'])) {
                /* 该位还有未尝试的字母，取当前字母继续向低位推进 */
                py_comp[index] = py9_map[k9_input[index] - '2'][flag];
                mark[index] = mark[index] + 1;
                index++;
            }
            else {
                /* 该位字母已穷尽，重置标记并回溯到上一位 */
                mark[index] = 0;
                index--;
            }
        }
    }

    /* 找到合法拼音后，更新已输入字符计数与合法拼音总数 */
    if(count > 0) {
        pinyin_ime->ta_count++;
        pinyin_ime->k9_legal_py_count = count;
    }
}

/**
 * 校验一个拼音串是否存在于字典中(前缀匹配)。
 * 实现与 pinyin_search_matching 类似，但只返回是否存在(true/false)。
 */
static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ = NULL;
    uint8_t index = 0, len = 0, offset = 0;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return false;
    if(*py_str == 'i')     return false;
    if(*py_str == 'u')     return false;
    if(*py_str == 'v')     return false;

    offset = py_str[0] - 'a';
    len = strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];
    count = pinyin_ime->py_num[offset];

    while(count--) {
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;
            }
        }

        if(len == 1 || index == len) {
            return true;   /* 前缀匹配成功 */
        }
        cpHZ++;
    }
    return false;
}

/**
 * K9 模式：把合法拼音链表的头几个拼音填充到候选按钮，
 * 并把第一个合法拼音写入 input_char、同步到 textarea 作为预览。
 */
static void pinyin_k9_fill_cand(lv_obj_t * obj)
{
    static uint16_t len = 0;   /* 上次填充时的合法拼音总数，用于判断是否需要重填 */
    uint16_t index = 0, tmp_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    tmp_len = pinyin_ime->k9_legal_py_count;

    /* 合法拼音总数发生变化时，清空候选缓冲并重置翻页按钮 */
    if(tmp_len != len) {
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
        len = tmp_len;
    }

    /* 从头遍历链表，把拼音串填入候选按钮(最多一屏) */
    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
    strcpy(pinyin_ime->input_char, ll_index->py_str);   /* 默认取第一个拼音作为预览 */
    while(ll_index) {
        if((index >= LV_IME_PINYIN_K9_CAND_TEXT_NUM) || \
           (index >= pinyin_ime->k9_legal_py_count))
            break;

        strcpy(lv_pinyin_k9_cand_str[index], ll_index->py_str);
        ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
        index++;
    }
    pinyin_ime->k9_py_ll_pos = index;   /* 记录当前候选填充到的链表位置 */

    /* 同步预览：先删掉 textarea 中旧的编码串，再写入新的合法拼音 */
    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    for(index = 0; index < pinyin_ime->k9_input_str_len; index++) {
        lv_textarea_del_char(ta);
    }
    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char);
    lv_textarea_add_text(ta, pinyin_ime->input_char);
}

/**
 * K9 模式候选翻页。
 * @param dir 0 = 上一页，1 = 下一页
 * 通过 k9_py_ll_pos 记录当前页在合法拼音链表中的位置，
 * 翻页时从链表对应位置重新取出一屏拼音填入候选按钮。
 */
static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    uint16_t ll_len =  _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);

    /* 只有合法拼音超过一屏时才需要翻页 */
    if((ll_len > LV_IME_PINYIN_K9_CAND_TEXT_NUM) && (pinyin_ime->k9_legal_py_count > LV_IME_PINYIN_K9_CAND_TEXT_NUM)) {
        ime_pinyin_k9_py_str_t * ll_index = NULL;
        int count = 0;

        /* 先把链表指针定位到当前页的起始位置(k9_py_ll_pos) */
        ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
        while(ll_index) {
            if(count >= pinyin_ime->k9_py_ll_pos)   break;

            ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
            count++;
        }

        /* 已在链表末尾却还要翻下一页，直接返回 */
        if((NULL == ll_index) && (dir == 1))   return;

        /* 清空候选缓冲并重置翻页按钮 */
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");

        // next page
        if(dir == 1) {
            /* 下一页：从当前位置向后取一屏拼音 */
            count = 0;
            while(ll_index) {
                if(count >= (LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1))
                    break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                count++;
            }
            pinyin_ime->k9_py_ll_pos += count - 1;
        }
        // previous page
        else {
            /* 上一页：从当前位置向前回退一屏拼音 */
            count = LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1;
            ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
            while(ll_index) {
                if(count < 0)  break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
                count--;
            }

            if(pinyin_ime->k9_py_ll_pos > LV_IME_PINYIN_K9_CAND_TEXT_NUM)
                pinyin_ime->k9_py_ll_pos -= 1;
        }

        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
    }
}

#endif  /*LV_IME_PINYIN_USE_K9_MODE*/

#endif  /*LV_USE_IME_PINYIN*/
