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
    {"b", "不吧把被本"},
    {"ba", "吧八巴捌叭芭笆疤拔跋靶把坝霸罢爸扒耙"},
    {"bai", "白摆佰败拜柏百稗伯"},
    {"ban", "半办绊扮拌伴瓣般斑班搬扳颁板版"},
    {"bang", "邦帮梆榜绑棒镑傍谤膀磅蚌"},
    {"bao", "包保饱宝抱报豹鲍爆剥薄暴刨炮曝堡苞胞褒雹"},
    {"bei", "杯碑悲卑北辈背贝钡倍狈备惫焙被"},
    {"ben", "本奔笨苯犇"},
    {"beng", "崩绷甭泵蹦迸蚌"},
    {"bi", "逼鼻比鄙笔彼碧蓖蔽毕毙毖币庇痹闭敝弊必壁避陛辟臂秘"},
    {"bian", "扁便遍边编变贬辨辩辫鞭卞"},
    {"biao", "标彪膘表飙镖婊裱"},
    {"bie", "鳖憋别瘪"},
    {"bin", "彬斌濒滨宾摈"},
    {"bing", "兵冰柄丙秉饼炳病并屏"},
    {"bo", "玻菠播拨钵博勃搏铂箔帛舶脖膊渤驳柏剥薄波泊卜般伯"},
    {"bu", "不捕哺补埠布步簿部怖卜埔"},
    {"c", "从次此才存"},
    {"ca", "擦嚓拆"},
    {"cai", "猜裁材才财睬踩采彩菜蔡"},
    {"can", "参餐残惭惨灿蚕掺"},
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
    {"d", "的得地都多"},
    {"da", "大打达搭答哒妲耷沓"},
    {"dai", "戴带待代呆袋歹贷傣殆逮怠"},
    {"dan", "耽担丹郸胆旦氮但惮淡诞蛋掸弹石单"},
    {"dang", "当挡党荡档"},
    {"dao", "刀捣蹈倒岛祷导到稻悼道盗"},
    {"de", "德得的地の"},
    {"dei", "得嘚"},
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
    {"e", "额饿峨鹅俄讹娥厄扼遏鄂阿蛾恶"},
    {"en", "嗯恩摁"},
    {"er", "二而耳尔儿饵洱贰"},
    {"f", "发法方分放"},
    {"fa", "发罚筏伐乏阀法珐"},
    {"fan", "返反饭帆翻番蕃犯繁泛樊矾钒凡烦范贩犯"},
    {"fang", "坊芳方肪房防妨仿访纺放"},
    {"fei", "菲非啡飞肥匪诽吠肺废沸费"},
    {"fen", "芬酚吩氛分纷坟焚汾粉奋份忿愤粪"},
    {"feng", "风丰封枫蜂峰锋疯烽逢缝讽奉凤冯"},
    {"fo", "佛"},
    {"fou", "否缶"},
    {"fu", "夫敷肤孵扶辐幅氟符伏俘服浮涪福袱弗甫抚辅俯釜斧腑府腐赴副覆赋复傅付阜父腹负富讣附妇缚咐佛拂脯"},
    {"g", "个过更工高"},
    {"ga", "噶嘎夹咖"},
    {"gai", "该改概钙溉盖芥"},
    {"gan", "干甘杆柑竿肝赶感秆敢赣乾"},
    {"gang", "冈刚钢缸肛纲岗港杠扛"},
    {"gao", "篙皋高膏羔糕搞稿镐告"},
    {"ge", "哥歌搁戈鸽疙割葛格阁隔铬个各胳革蛤咯"},
    {"gei", "给"},
    {"gen", "根跟亘艮哏"},
    {"geng", "更梗耕庚羹耿埂粳颈"},
    {"gong", "工共公功恭弓龚供躬宫巩拱贡汞"},
    {"gou", "钩勾沟苟狗垢构购够"},
    {"gu", "辜菇咕箍估沽孤姑古蛊骨股故顾固雇鼓谷贾"},
    {"gua", "刮瓜剐寡挂褂"},
    {"guai", "乖拐怪"},
    {"guan", "棺关官冠观管馆罐惯灌贯纶"},
    {"guang", "光逛广胱咣"},
    {"gui", "瑰规圭归闺轨鬼诡癸桂柜跪贵刽硅傀炔龟"},
    {"gun", "滚棍辊"},
    {"guo", "锅郭国果裹过涡"},
    {"g", "和好会还后"},
    {"ha", "蛤哈"},
    {"hai", "还孩海害氦骸亥骇咳"},
    {"han", "汉汗焊酣憨邯韩含涵寒函喊罕翰撼捍旱憾"},
    {"hang", "杭航夯吭巷行"},
    {"hao", "好号壕嚎豪毫郝浩耗镐貉"},
    {"he", "喝荷菏禾何盒阂河赫褐鹤贺核合涸吓呵貉和"},
    {"hei", "黑嘿"},
    {"hen", "痕很狠恨"},
    {"heng", "亨横衡恒哼行"},
    {"hong", "轰哄烘虹鸿洪宏弘红"},
    {"hou", "后候喉侯猴吼厚"},
    {"hu", "呼乎忽瑚壶葫胡蝴狐糊湖弧虎护互沪户唬和"},
    {"hua", "花华猾画化话哗滑划"},
    {"huai", "槐怀淮徊坏"},
    {"huan", "欢环还桓缓换患唤痪豢焕涣宦幻"},
    {"huang", "荒慌黄磺蝗簧皇凰惶煌晃幌恍谎"},
    {"hui", "灰挥辉徽恢蛔回毁悔慧卉惠晦贿秽烩汇讳诲绘会"},
    {"hun", "昏婚魂浑混荤"},
    {"huo", "或活伙火和获惑霍货祸豁"},
    {"i", ""},
    {"ji", "击圾基机畸积箕肌饥迹激讥鸡姬绩吉极棘辑籍集及急疾汲即嫉级挤几脊己蓟技冀季伎剂悸济寄寂计记既忌际妓继纪给稽缉祭藉期奇齐系"},
    {"jia", "加假夹家佳甲架价驾嘉荚颊枷钾稼嫁贾茄缴"},
    {"jian", "歼监坚尖笺间煎兼肩艰奸缄茧检柬碱拣捡简俭剪减荐鉴践贱键箭件健舰剑饯渐溅涧建槛见浅"},
    {"jiang", "将讲江降酱奖匠浆僵姜桨疆蒋强"},
    {"jiao", "角较叫脚交觉教椒礁焦胶郊浇骄娇轿窖蕉嚼搅铰狡饺绞酵校矫侥缴剿"},
    {"jie", "揭接皆秸街阶截劫节杰捷睫竭洁结姐戒界借介疥诫届桔解藉芥"},
    {"jin", "今仅尽斤金进禁近巾筋津劲襟紧锦谨晋烬浸靳"},
    {"jing", "惊精敬警静境净茎睛晶鲸京经井镜径痉靖竟竞劲荆兢粳景颈"},
    {"jiong", "炯窘囧"},
    {"jiu", "就揪究纠玖韭久灸九酒厩救旧臼舅咎疚"},
    {"ju", "句居咀拘剧车桔狙菊局矩举沮聚拒据巨具距踞锯俱惧炬鞠疽驹"},
    {"juan", "捐鹃娟倦眷绢卷圈"},
    {"jue", "撅攫抉掘倔爵决诀绝嚼觉角"},
    {"jun", "菌钧军君峻俊竣郡骏均浚"},
    {"ka", "卡喀咖咔"},
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
    {"l", "了来里两来"},
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
    {"m", "么没们名民"},
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
    {"n", "你呢那能年"},
    {"na", "拿钠纳呐那娜哪"},
    {"nai", "氖乃奶耐奈哪"},
    {"nan", "南男难"},
    {"nang", "囊"},
    {"nao", "挠脑恼闹淖"},
    {"ne", "呢哪"},
    {"nei", "馁内那哪"},
    {"nen", "嫩恁"},
    {"neng", "能"},
    {"ni", "你逆拟泥匿妮霓倪尼腻溺呢"},
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
    {"qi", "七起岂启弃齐乞其奇期欺戚妻凄柒沏棋脐旗祈祁骑企器气迄汽讫栖砌泣漆契歧崎畦"},
    {"qia", "恰卡掐洽"},
    {"qian", "牵扦钎千迁签仟谦黔钱钳前潜遣谴堑欠歉铅乾浅嵌纤"},
    {"qiang", "强抢墙呛枪腔羌蔷"},
    {"qiao", "锹敲悄桥乔侨巧撬翘峭俏窍壳橇瞧鞘"},
    {"qie", "且切窃茄怯"},
    {"qin", "钦侵秦琴勤芹擒禽寝亲沁"},
    {"qing", "请轻倾庆亲清情氢青卿擎晴氰顷"},
    {"qiong", "穷琼穹"},
    {"qiu", "秋丘邱球求囚酋泅"},
    {"qu", "趋曲躯屈驱渠取娶龋去区蛆趣"},
    {"quan", "颧权醛泉全痊拳犬券劝卷圈"},
    {"que", "缺瘸却鹊榷确炔雀"},
    {"qun", "裙群逡"},
    {"r", "人然让日任"},
    {"ran", "然燃冉染"},
    {"rang", "瓤壤攘嚷让"},
    {"rao", "饶扰绕"},
    {"re", "惹热"},
    {"ren", "壬仁人忍韧任认刃妊纫亻"},
    {"reng", "扔仍"},
    {"ri", "日"},
    {"rong", "容荣融熔溶冗戎茸蓉绒"},
    {"rou", "揉柔肉"},
    {"ru", "如入茹辱乳汝褥蠕儒孺"},
    {"ruan", "软阮"},
    {"rui", "锐蕊瑞睿芮枘"},
    {"run", "闰润"},
    {"ruo", "弱若偌叒"},
    {"s", "三四什是时似说"},
    {"sa", "撒洒萨仨"},
    {"sai", "赛塞鳃腮"},
    {"san", "三叁伞散"},
    {"sang", "桑嗓丧"},
    {"sao", "搔骚扫嫂梢臊缫瘙"},
    {"se", "瑟涩塞色铯啬"},
    {"sen", "森"},
    {"seng", "僧"},
    {"sha", "啥杀沙纱傻砂煞莎刹杉厦"},
    {"shai", "筛晒色"},
    {"shan", "单山闪删善扇衫讪擅陕煽赡膳汕缮杉栅珊苫姗疝鳝膻"},
    {"shang", "伤商赏晌上尚裳汤墒殇觞熵"},
    {"shao", "捎稍烧芍勺韶少哨邵绍鞘梢召"},
    {"she", "奢赊舌舍赦摄慑涉社设蛇拾折射"},
    {"shei", "谁"},
    {"shen", "什申伸身深神甚参肾慎绅审婶渗沈娠砷呻"},
    {"sheng", "声生甥牲升绳剩胜圣乘省盛"},
    {"shi", "是十失事世始式示实时试石拾食识市室适势师施湿诗什尸狮虱蚀史矢使屎驶士柿拭誓逝嗜噬仕侍释饰恃视匙氏似嘘殖峙"},
    {"shou", "受收手首守寿授售瘦兽熟"},
    {"shu", "蔬枢梳殊抒输叔舒淑疏书赎孰薯暑曙署蜀黍鼠述树束戍竖墅庶漱恕熟属术数"},
    {"shua", "刷耍"},
    {"shuai", "摔甩帅衰率蟀"},
    {"shuan", "栓拴涮闩"},
    {"shuang", "霜双爽孀"},
    {"shui", "水睡税谁说氵"},
    {"shun", "顺瞬吮舜"},
    {"shuo", "硕朔烁数说槊蒴铄"},
    {"si", "四似撕嘶私司丝死斯肆寺嗣饲巳食思伺"},
    {"song", "松耸怂颂送宋讼诵嵩淞"},
    {"sou", "嗖搜擞嗽艘馊叟薮飕"},
    {"su", "苏酥俗素速粟塑溯诉肃宿夙簌窣愫稣僳"},
    {"suan", "酸蒜算"},
    {"sui", "虽遂随碎岁隧穗祟绥髓尿隋"},
    {"sun", "孙损笋隼榫狲"},
    {"suo", "所缩锁唆索琐莎蓑梭娑羧嗦惢"},
    {"ta", "他她它踏塔塌獭挞蹋沓拓祂榻铊"},
    {"tai", "太抬态台泰胎苔汰钛跆酞肽薹呔"},
    {"tan", "探叹弹摊谈坍贪瘫滩坛檀痰潭谭坦毯袒碳炭"},
    {"tang", "躺烫汤趟堂糖塘搪淌棠膛唐敞倘"},
    {"tao", "桃逃掏讨套涛滔陶萄淘韬绦"},
    {"te", "特忒忑"},
    {"teng", "藤腾疼誊滕"},
    {"ti", "提体替梯题蹄啼踢剔锑惕嚏涕剃屉"},
    {"tian", "天添填田甜恬舔腆兲殄钿"},
    {"tiao", "调挑条眺跳迢窕"},
    {"tie", "贴铁帖餮"},
    {"ting", "停挺听厅婷霆亭庭町廷艇烃汀"},
    {"tong", "同通统痛铜瞳彤童桶捅筒侗恫桐酮"},
    {"tou", "偷投头透骰"},
    {"tu", "突图徒途涂土吐凸屠兔秃余荼"},
    {"tuan", "湍团抟"},
    {"tui", "推退腿颓褪蜕"},
    {"tun", "吞屯臀囤鲀豚"},
    {"tuo", "拖托脱坨拓椭妥唾驮沱鸵陀驼砣"},
    {"u", ""},
    {"v", ""},
    {"w", "我为无玩完五万"},
    {"wa", "挖哇蛙洼娃瓦袜佤娲凹"},
    {"wai", "外歪崴"},
    {"wan", "万弯完晚挽湾玩碗顽丸宛蜿惋婉腕蔓皖烷豌"},
    {"wang", "望忘网亡汪王枉往旺妄罔惘魍"},
    {"wei", "为未微危维尾味唯惟围委伟慰伪违威卫韦苇萎纬畏胃喂魏位渭谓蔚尉桅潍巍"},
    {"wen", "问闻温蚊文稳纹吻紊瘟雯汶"},
    {"weng", "嗡翁瓮"},
    {"wo", "我喔握窝卧沃涡挝蜗斡"},
    {"wu", "五无勿呜物务乌污屋芜吾吴武捂午舞伍侮坞戊雾晤悟误毋巫诬恶梧钨"},
    {"x", "下学想小现"},
    {"xi", "西系嘻喜洗戏细昔惜习席息希吸析晰稀夕悉栖熙袭牺汐茜硒矽烯曦犀檄铣"},
    {"xia", "下夏侠吓瞎匣霞辖暇峡狭虾厦"},
    {"xian", "先现贤显闲鲜咸仙衔线纤弦嫌险献县腺馅羡宪陷限舷涎掀锨铣"},
    {"xiang", "相厢镶香箱襄湘乡翔祥详想响享项橡像向象降巷"},
    {"xiao", "小笑消校宵晓效削哮销孝肖啸嚣淆萧硝霄"},
    {"xie", "些写歇鞋协血携胁谐械卸泄泻谢屑解挟邪斜楔蝎懈蟹"},
    {"xin", "心信新欣辛芯衅薪锌忻"},
    {"xing", "行兴醒星型刑形幸杏性姓省邢腥猩惺"},
    {"xiong", "兄凶胸匈汹雄熊"},
    {"xiu", "休修羞朽嗅锈秀袖绣臭宿"},
    {"xu", "许须需虚续徐嘘序叙墟蓄酗恤絮绪戌畜吁旭婿"},
    {"xuan", "选悬旋宣轩喧眩绚玄癣"},
    {"xue", "雪削血学穴靴薛"},
    {"xun", "寻勋循旬询巡汛训讯熏驯逊迅殉浚"},
    {"y", "一有也要与"},
    {"ya", "呀压押鸦鸭丫牙蚜衙涯雅哑亚讶芽崖轧"},
    {"yan", "焉阉淹盐严研蜒岩延言颜阎炎沿奄掩眼衍演艳堰燕厌砚雁唁彦焰宴谚验铅咽烟殷"},
    {"yang", "样阳养洋扬羊杨央氧仰秧殃鸯佯疡漾"},
    {"yao", "要药邀腰妖瑶摇尧遥窑谣姚咬舀耀约钥侥"},
    {"ye", "椰噎耶爷野冶也页业夜咽掖叶腋液拽曳"},
    {"yi", "一已以遗壹医依易伊衣夷乙艺忆亿亦移仪疑宜姨椅倚矣抑邑役臆逸意毅义益溢诣议谊译异揖蚁沂颐胰肄疫铱裔彝翼翌绎屹"},
    {"yin", "引印因音迎阴吟银饮隐淫寅姻殷尹茵荫"},
    {"ying", "应硬英赢映鹰缨莹营荧盈影蝇颖樱萤婴"},
    {"yo", "哟唷"},
    {"yong", "用勇永泳拥佣咏臃痈庸雍踊蛹恿涌"},
    {"you", "又有右友由幽优悠忧尤邮铀犹油游酉佑釉诱"},
    {"yu", "于与语雨鱼育遇欲预余愉愚玉域羽宇浴裕誉吁迂盂榆虞舆逾渝渔隅予娱屿禹羽芋郁喻峪御愈狱寓驭尉俞"},
    {"yuan", "鸳渊冤元垣袁原援辕园圆猿源缘远苑愿怨院员"},
    {"yue", "曰越跃岳粤月悦阅乐约钥"},
    {"yun", "耘云郧匀陨允运蕴酝晕韵孕均"},
    {"z", "在这总只中"},
    {"za", "匝砸杂扎咱咋"},
    {"zai", "栽哉灾宰载再在仔"},
    {"zan", "暂赞攒咱"},
    {"zang", "赃脏葬藏"},
    {"zao", "遭糟藻枣早澡蚤躁噪造皂灶燥凿"},
    {"ze", "责则泽择啧仄"},
    {"zei", "贼"},
    {"zen", "怎"},
    {"zeng", "增憎赠曾综"},
    {"zha", "渣炸铡咋闸眨榨乍诈查扎喳栅柞札轧"},
    {"zhai", "斋债寨翟祭择摘宅窄侧"},
    {"zhan", "瞻毡詹沾盏斩辗崭展蘸栈占战站湛绽颤粘"},
    {"zhang", "樟章彰漳张掌涨杖丈帐账仗胀瘴障长"},
    {"zhao", "招昭找沼赵照罩兆肇朝召爪着"},
    {"zhe", "遮哲蛰辙者蔗浙折锗这着"},
    {"zhen", "珍斟真甄砧臻贞针侦枕疹诊震振镇阵帧"},
    {"zheng", "蒸挣睁征狰争怔整拯正政症郑证"},
    {"zhi", "芝支蜘知肢脂汁之织职直植执值侄址指止趾只旨纸志挚掷至致置帜制智秩稚质炙痔滞治窒识枝吱殖峙"},
    {"zhong", "中种重众钟衷终肿忠仲盅"},
    {"zhou", "周州洲舟诌轴肘帚咒皱宙昼骤粥"},
    {"zhu", "助住珠祝逐主株朱猪诸诛竹烛煮拄瞩嘱柱蛛蛀贮铸筑注驻属著"},
    {"zhua", "抓爪"},
    {"zhuai", "拽"},
    {"zhuan", "专转砖传赚撰篆"},
    {"zhuang", "装撞状庄壮桩幢妆"},
    {"zhui", "追坠缀椎锥赘"},
    {"zhun", "准谆"},
    {"zhuo", "着捉卓拙桌茁啄灼浊琢酌"},
    {"zi", "自子字紫仔兹咨资姿滋淄孜籽滓渍"},
    {"zong", "总踪综宗纵棕鬃"},
    {"zou", "走奏揍邹"},
    {"zu", "足阻组卒租族祖诅"},
    {"zuan", "钻纂攥"},
    {"zui", "嘴醉最罪"},
    {"zun", "尊遵樽鳟"},
    {"zuo", "做作昨左坐座佐撮琢柞"},
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

static void lv_ime_pinyin_kb_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    static const char * k9_py_map[8] = {"abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"};
#endif

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint16_t btn_id  = lv_btnmatrix_get_selected_btn(kb);
        if(btn_id == LV_BTNMATRIX_BTN_NONE) return;

        const char * txt = lv_btnmatrix_get_btn_text(kb, lv_btnmatrix_get_selected_btn(kb));
        if(txt == NULL) return;

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

        if(strcmp(txt, "Enter") == 0 || strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
            pinyin_ime_clear_data(obj);
            lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            // del input char
            if(pinyin_ime->ta_count > 0) {
                if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26)
                    pinyin_ime->input_char[pinyin_ime->ta_count - 1] = '\0';
#if LV_IME_PINYIN_USE_K9_MODE
                else if (pinyin_ime->mode == LV_IME_PINYIN_MODE_K9)
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count - 1] = '\0';
#endif

                pinyin_ime->ta_count -= 1;
                printf("[ime] ta_count=%d\n", pinyin_ime->ta_count);
                if(pinyin_ime->ta_count <= 0) {
                    pinyin_ime_clear_data(obj);
                }
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                    pinyin_input_proc(obj);
                }
#if LV_IME_PINYIN_USE_K9_MODE
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
                    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char) - 1;
                    pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
                    pinyin_k9_fill_cand(obj);
                    pinyin_input_proc(obj);
                }
#endif
            }
        }
        else if((strcmp(txt, "ABC") == 0) || (strcmp(txt, "abc") == 0) || (strcmp(txt, "1#") == 0)) {
            pinyin_ime_clear_data(obj);
            return;
        }
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
        else if(strcmp(txt, LV_SYMBOL_OK) == 0) {
            pinyin_ime_clear_data(obj);
        }
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) && (txt[0] >= 'a' && txt[0] <= 'z')) {
            strcat(pinyin_ime->input_char, txt);
            pinyin_input_proc(obj);
            pinyin_ime->ta_count++;
        }
#if LV_IME_PINYIN_USE_K9_MODE
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) && (txt[0] >= 'a' && txt[0] <= 'z')) {
            for(uint16_t i = 0; i < 8; i++) {
                if((strcmp(txt, k9_py_map[i]) == 0) || (strcmp(txt, "abc ") == 0)) {
                    if(strcmp(txt, "abc ") == 0)    pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]) + 1;
                    else                            pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]);
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count] = 50 + i;

                    break;
                }
            }
            pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
            pinyin_k9_fill_cand(obj);
            pinyin_input_proc(obj);
        }
        else if(strcmp(txt, LV_SYMBOL_LEFT) == 0) {
            pinyin_k9_cand_page_proc(obj, 0);
        }
        else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            pinyin_k9_cand_page_proc(obj, 1);
        }
#endif
    }
}

static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cand_panel = lv_event_get_target(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(cand_panel);
        if(id == 0) {
            pinyin_page_proc(obj, 0);
            return;
        }
        if(id == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            pinyin_page_proc(obj, 1);
            return;
        }

        const char * txt = lv_btnmatrix_get_btn_text(cand_panel, id);
        lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
        uint16_t index = 0;
        for(index = 0; index < pinyin_ime->ta_count; index++)
            lv_textarea_del_char(ta);

        lv_textarea_add_text(ta, txt);

        pinyin_ime_clear_data(obj);
    }
}

static void pinyin_input_proc(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime->cand_str = pinyin_search_matching(obj, pinyin_ime->input_char, &pinyin_ime->cand_num);
    if(pinyin_ime->cand_str == NULL) {
        return;
    }

    pinyin_ime->py_page = 0;

    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    // fill buf
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[i * 3 + j];
        }
    }

    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
}

static void pinyin_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t page_num = pinyin_ime->cand_num / LV_IME_PINYIN_CAND_TEXT_NUM;
    uint16_t sur = pinyin_ime->cand_num % LV_IME_PINYIN_CAND_TEXT_NUM;

    if(dir == 0) {
        if(pinyin_ime->py_page) {
            pinyin_ime->py_page--;
        }
    }
    else {
        if(sur == 0) {
            page_num -= 1;
        }
        if(pinyin_ime->py_page < page_num) {
            pinyin_ime->py_page++;
        }
        else return;
    }

    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    // fill buf
    uint16_t offset = pinyin_ime->py_page * (3 * LV_IME_PINYIN_CAND_TEXT_NUM);
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
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

static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    char headletter = 'a';
    uint16_t offset_sum = 0;
    uint16_t offset_count = 0;
    uint16_t letter_calc = 0;

    pinyin_ime->dict = dict;

    for(uint16_t i = 0; ; i++) {
        if((NULL == (dict[i].py)) || (NULL == (dict[i].py_mb))) {
            headletter = dict[i - 1].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc] = offset_count;
            break;
        }

        if(headletter == (dict[i].py[0])) {
            offset_count++;
        }
        else {
            headletter = dict[i].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc - 1] = offset_count;
            offset_sum += offset_count;
            pinyin_ime->py_pos[letter_calc] = offset_sum;

            offset_count = 1;
        }
    }
}

static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ;
    uint8_t index, len = 0, offset;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return NULL;
    if(*py_str == 'i')     return NULL;
    if(*py_str == 'u')     return NULL;
    if(*py_str == 'v')     return NULL;

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

        // perfect match
        if(len == 1 || index == len) {
            // The Chinese character in UTF-8 encoding format is 3 bytes
            * cand_num = strlen((const char *)(cpHZ->py_mb)) / 3;
            return (char *)(cpHZ->py_mb);
        }
        cpHZ++;
    }
    return NULL;
}

static void pinyin_ime_clear_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
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

static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[])
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t len = strlen(k9_input);

    if((len == 0) || (len >= LV_IME_PINYIN_K9_MAX_INPUT)) {
        return;
    }

    char py_comp[LV_IME_PINYIN_K9_MAX_INPUT] = {0};
    int mark[LV_IME_PINYIN_K9_MAX_INPUT] = {0};
    int index = 0;
    int flag = 0;
    int count = 0;

    uint32_t ll_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    ll_len = _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);
    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);

    while(index != -1) {
        if(index == len) {
            if(pinyin_k9_is_valid_py(obj, py_comp)) {
                if((count >= ll_len) || (ll_len == 0)) {
                    ll_index = _lv_ll_ins_tail(&pinyin_ime->k9_legal_py_ll);
                    strcpy(ll_index->py_str, py_comp);
                }
                else if((count < ll_len)) {
                    strcpy(ll_index->py_str, py_comp);
                    ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                }
                count++;
            }
            index--;
        }
        else {
            flag = mark[index];
            if(flag < strlen(py9_map[k9_input[index] - '2'])) {
                py_comp[index] = py9_map[k9_input[index] - '2'][flag];
                mark[index] = mark[index] + 1;
                index++;
            }
            else {
                mark[index] = 0;
                index--;
            }
        }
    }

    if(count > 0) {
        pinyin_ime->ta_count++;
        pinyin_ime->k9_legal_py_count = count;
    }
}

/*true: visible; false: not visible*/
static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ = NULL;
    uint8_t index = 0, len = 0, offset = 0;
    //uint16_t ret = 1;
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

        // perfect match
        if(len == 1 || index == len) {
            return true;
        }
        cpHZ++;
    }
    return false;
}

static void pinyin_k9_fill_cand(lv_obj_t * obj)
{
    static uint16_t len = 0;
    uint16_t index = 0, tmp_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    tmp_len = pinyin_ime->k9_legal_py_count;

    if(tmp_len != len) {
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
        len = tmp_len;
    }

    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
    strcpy(pinyin_ime->input_char, ll_index->py_str);
    while(ll_index) {
        if((index >= LV_IME_PINYIN_K9_CAND_TEXT_NUM) || \
           (index >= pinyin_ime->k9_legal_py_count))
            break;

        strcpy(lv_pinyin_k9_cand_str[index], ll_index->py_str);
        ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
        index++;
    }
    pinyin_ime->k9_py_ll_pos = index;

    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    for(index = 0; index < pinyin_ime->k9_input_str_len; index++) {
        lv_textarea_del_char(ta);
    }
    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char);
    lv_textarea_add_text(ta, pinyin_ime->input_char);
}

static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    uint16_t ll_len =  _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);

    if((ll_len > LV_IME_PINYIN_K9_CAND_TEXT_NUM) && (pinyin_ime->k9_legal_py_count > LV_IME_PINYIN_K9_CAND_TEXT_NUM)) {
        ime_pinyin_k9_py_str_t * ll_index = NULL;
        int count = 0;

        ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
        while(ll_index) {
            if(count >= pinyin_ime->k9_py_ll_pos)   break;

            ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
            count++;
        }

        if((NULL == ll_index) && (dir == 1))   return;

        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");

        // next page
        if(dir == 1) {
            count = 0;
            while(ll_index) {
                if(count >= (LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1))
                    break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
                count++;
            }
            pinyin_ime->k9_py_ll_pos += count - 1;

        }
        // previous page
        else {
            count = LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1;
            ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
            while(ll_index) {
                if(count < 0)  break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the previous list*/
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
