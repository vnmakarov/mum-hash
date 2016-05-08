#if defined(SHA2)

#include "sha512.h"
void sha512_test (const void *msg, int len, void *out) {
  sha512_ctx ctx;
  
  rhash_sha512_init (&ctx);
  rhash_sha512_update (&ctx, msg, len);
  rhash_sha512_final (&ctx, out);
}

#define test sha512_test

#elif defined(SHA3)

#include "sha3.h"
void sha3_512_test (const void *msg, int len, void *out) {
  sha3_ctx ctx;
  
  rhash_sha3_512_init (&ctx);
  rhash_sha3_update (&ctx, msg, len);
  rhash_sha3_final (&ctx, out);
}

#define test sha3_512_test

#elif defined(MUM512)

#include "mum512.h"

void mum512_test (const void *msg, int len, void *out) {
  mum512_hash (msg, len, out);
}

#define test mum512_test

#else
#error "I don't know what to test"
#endif

static const char msg[] =
"SATURDAY morning was come, and all the summer world was bright and\n\
fresh, and brimming with life. There was a song in every heart; and if\n\
the heart was young the music issued at the lips. There was cheer in\n\
every face and a spring in every step. The locust-trees were in bloom\n\
and the fragrance of the blossoms filled the air. Cardiff Hill, beyond\n\
the village and above it, was green with vegetation and it lay just far\n\
enough away to seem a Delectable Land, dreamy, reposeful, and inviting.\n\
\n\
Tom appeared on the sidewalk with a bucket of whitewash and a\n\
long-handled brush. He surveyed the fence, and all gladness left him and\n\
a deep melancholy settled down upon his spirit. Thirty yards of board\n\
fence nine feet high. Life to him seemed hollow, and existence but a\n\
burden. Sighing, he dipped his brush and passed it along the topmost\n\
plank; repeated the operation; did it again; compared the insignificant\n\
whitewashed streak with the far-reaching continent of unwhitewashed\n\
fence, and sat down on a tree-box discouraged. Jim came skipping out at\n\
the gate with a tin pail, and singing Buffalo Gals. Bringing water from\n\
the town pump had always been hateful work in Tom's eyes, before, but\n\
now it did not strike him so. He remembered that there was company at\n\
the pump. White, mulatto, and negro boys and girls were always there\n\
waiting their turns, resting, trading playthings, quarrelling, fighting,\n\
skylarking. And he remembered that although the pump was only a hundred\n\
and fifty yards off, Jim never got back with a bucket of water under an\n\
hour--and even then somebody generally had to go after him. Tom said:\n\
\n\
\"Say, Jim, I'll fetch the water if you'll whitewash some.\"\n\
\n\
Jim shook his head and said:\n\
\n\
\"Can't, Mars Tom. Ole missis, she tole me I got to go an' git dis water\n\
an' not stop foolin' roun' wid anybody. She say she spec' Mars Tom gwine\n\
to ax me to whitewash, an' so she tole me go 'long an' 'tend to my own\n\
business--she 'lowed _she'd_ 'tend to de whitewashin'.\"\n\
\n\
\"Oh, never you mind what she said, Jim. That's the way she always talks.\n\
Gimme the bucket--I won't be gone only a a minute. _She_ won't ever\n\
know.\"\n\
\n\
\"Oh, I dasn't, Mars Tom. Ole missis she'd take an' tar de head off'n me.\n\
'Deed she would.\"\n\
\n\
\"_She_! She never licks anybody--whacks 'em over the head with her\n\
thimble--and who cares for that, I'd like to know. She talks awful, but\n\
talk don't hurt--anyways it don't if she don't cry. Jim, I'll give you a\n\
marvel. I'll give you a white alley!\"\n\
\n\
Jim began to waver.\n\
\n\
\"White alley, Jim! And it's a bully taw.\"\n\
\n\
\"My! Dat's a mighty gay marvel, I tell you! But Mars Tom I's powerful\n\
'fraid ole missis--\"\n\
\n\
\"And besides, if you will I'll show you my sore toe.\"\n\
\n\
Jim was only human--this attraction was too much for him. He put down\n\
his pail, took the white alley, and bent over the toe with absorbing\n\
interest while the bandage was being unwound. In another moment he\n\
was flying down the street with his pail and a tingling rear, Tom was\n\
whitewashing with vigor, and Aunt Polly was retiring from the field with\n\
a slipper in her hand and triumph in her eye.\n\
\n\
But Tom's energy did not last. He began to think of the fun he had\n\
planned for this day, and his sorrows multiplied. Soon the free boys\n\
would come tripping along on all sorts of delicious expeditions, and\n\
they would make a world of fun of him for having to work--the very\n\
thought of it burnt him like fire. He got out his worldly wealth and\n\
examined it--bits of toys, marbles, and trash; enough to buy an exchange\n\
of _work_, maybe, but not half enough to buy so much as half an hour\n\
of pure freedom. So he returned his straitened means to his pocket, and\n\
gave up the idea of trying to buy the boys. At this dark and hopeless\n\
moment an inspiration burst upon him! Nothing less than a great,\n\
magnificent inspiration.\n\
\n\
He took up his brush and went tranquilly to work. Ben Rogers hove in\n\
sight presently--the very boy, of all boys, whose ridicule he had been\n\
dreading. Ben's gait was the hop-skip-and-jump--proof enough that his\n\
heart was light and his anticipations high. He was eating an apple, and\n\
giving a long, melodious whoop, at intervals, followed by a deep-toned\n\
ding-dong-dong, ding-dong-dong, for he was personating a steamboat. As\n\
he drew near, he slackened speed, took the middle of the street, leaned\n\
far over to starboard and rounded to ponderously and with laborious pomp\n\
and circumstance--for he was personating the Big Missouri, and considered\n\
himself to be drawing nine feet of water. He was boat and captain and\n\
engine-bells combined, so he had to imagine himself standing on his own\n\
hurricane-deck giving the orders and executing them:\n\
\n\
\"Stop her, sir! Ting-a-ling-ling!\" The headway ran almost out, and he\n\
drew up slowly toward the sidewalk.\n\
\n\
\"Ship up to back! Ting-a-ling-ling!\" His arms straightened and stiffened\n\
down his sides.\n\
\n\
\"Set her back on the stabboard! Ting-a-ling-ling! Chow! ch-chow-wow!\n\
Chow!\" His right hand, mean-time, describing stately circles--for it was\n\
representing a forty-foot wheel.\n\
\n\
\"Let her go back on the labboard! Ting-a-ling-ling! Chow-ch-chow-chow!\"\n\
The left hand began to describe circles.\n\
\n\
\"Stop the stabboard! Ting-a-ling-ling! Stop the labboard! Come ahead on\n\
the stabboard! Stop her! Let your outside turn over slow! Ting-a-ling-ling!\n\
Chow-ow-ow! Get out that head-line! _lively_ now! Come--out with\n\
your spring-line--what're you about there! Take a turn round that stump\n\
with the bight of it! Stand by that stage, now--let her go! Done with\n\
the engines, sir! Ting-a-ling-ling! SH'T! S'H'T! SH'T!\" (trying the\n\
gauge-cocks).\n\
\n\
Tom went on whitewashing--paid no attention to the steamboat. Ben stared\n\
a moment and then said: \"_Hi-Yi! You're_ up a stump, ain't you!\"\n\
\n\
No answer. Tom surveyed his last touch with the eye of an artist, then\n\
he gave his brush another gentle sweep and surveyed the result, as\n\
before. Ben ranged up alongside of him. Tom's mouth watered for the\n\
apple, but he stuck to his work. Ben said:\n\
\n\
\"Hello, old chap, you got to work, hey?\"\n\
\n\
Tom wheeled suddenly and said:\n\
\n\
\"Why, it's you, Ben! I warn't noticing.\"\n\
\n\
\"Say--I'm going in a-swimming, I am. Don't you wish you could? But of\n\
course you'd druther _work_--wouldn't you? Course you would!\"\n\
\n\
Tom contemplated the boy a bit, and said:\n\
\n\
\"What do you call work?\"\n\
\n\
\"Why, ain't _that_ work?\"\n\
\n\
Tom resumed his whitewashing, and answered carelessly:\n\
\n\
\"Well, maybe it is, and maybe it ain't. All I know, is, it suits Tom\n\
Sawyer.\"\n\
\n\
\"Oh come, now, you don't mean to let on that you _like_ it?\"\n\
\n\
The brush continued to move.\n\
\n\
\"Like it? Well, I don't see why I oughtn't to like it. Does a boy get a\n\
chance to whitewash a fence every day?\"\n\
\n\
That put the thing in a new light. Ben stopped nibbling his apple.\n\
Tom swept his brush daintily back and forth--stepped back to note the\n\
effect--added a touch here and there--criticised the effect again--Ben\n\
watching every move and getting more and more interested, more and more\n\
absorbed. Presently he said:\n\
\n\
\"Say, Tom, let _me_ whitewash a little.\"\n\
\n\
Tom considered, was about to consent; but he altered his mind:\n\
\n\
\"No--no--I reckon it wouldn't hardly do, Ben. You see, Aunt Polly's awful\n\
particular about this fence--right here on the street, you know--but if it\n\
was the back fence I wouldn't mind and _she_ wouldn't. Yes, she's awful\n\
particular about this fence; it's got to be done very careful; I reckon\n\
there ain't one boy in a thousand, maybe two thousand, that can do it\n\
the way it's got to be done.\"\n\
\n\
\"No--is that so? Oh come, now--lemme just try. Only just a little--I'd let\n\
_you_, if you was me, Tom.\"\n\
\n\
\"Ben, I'd like to, honest injun; but Aunt Polly--well, Jim wanted to do\n\
it, but she wouldn't let him; Sid wanted to do it, and she wouldn't let\n\
Sid. Now don't you see how I'm fixed? If you was to tackle this fence\n\
and anything was to happen to it--\"\n\
\n\
\"Oh, shucks, I'll be just as careful. Now lemme try. Say--I'll give you\n\
the core of my apple.\"\n\
\n\
\"Well, here--No, Ben, now don't. I'm afeard--\"\n\
\n\
\"I'll give you _all_ of it!\"\n\
\n\
Tom gave up the brush with reluctance in his face, but alacrity in his\n\
heart. And while the late steamer Big Missouri worked and sweated in the\n\
sun, the retired artist sat on a barrel in the shade close by,\n\
dangled his legs, munched his apple, and planned the slaughter of more\n\
innocents. There was no lack of material; boys happened along every\n\
little while; they came to jeer, but remained to whitewash. By the time\n\
Ben was fagged out, Tom had traded the next chance to Billy Fisher for\n\
a kite, in good repair; and when he played out, Johnny Miller bought in\n\
for a dead rat and a string to swing it with--and so on, and so on, hour\n\
after hour. And when the middle of the afternoon came, from being a\n\
poor poverty-stricken boy in the morning, Tom was literally rolling in\n\
wealth. He had besides the things before mentioned, twelve marbles, part\n\
of a jews-harp, a piece of blue bottle-glass to look through, a spool\n\
cannon, a key that wouldn't unlock anything, a fragment of chalk, a\n\
glass stopper of a decanter, a tin soldier, a couple of tadpoles,\n\
six fire-crackers, a kitten with only one eye, a brass door-knob, a\n\
dog-collar--but no dog--the handle of a knife, four pieces of orange-peel,\n\
and a dilapidated old window sash.\n\
\n\
He had had a nice, good, idle time all the while--plenty of company--and\n\
the fence had three coats of whitewash on it! If he hadn't run out of\n\
whitewash he would have bankrupted every boy in the village.\n\
\n\
Tom said to himself that it was not such a hollow world, after all. He\n\
had discovered a great law of human action, without knowing it--namely,\n\
that in order to make a man or a boy covet a thing, it is only necessary\n\
to make the thing difficult to attain. If he had been a great and\n\
wise philosopher, like the writer of this book, he would now have\n\
comprehended that Work consists of whatever a body is _obliged_ to do,\n\
and that Play consists of whatever a body is not obliged to do. And\n\
this would help him to understand why constructing artificial flowers or\n\
performing on a tread-mill is work, while rolling ten-pins or climbing\n\
Mont Blanc is only amusement. There are wealthy gentlemen in England\n\
who drive four-horse passenger-coaches twenty or thirty miles on a\n\
daily line, in the summer, because the privilege costs them considerable\n\
money; but if they were offered wages for the service, that would turn\n\
it into work and then they would resign.\n\
\n\
The boy mused awhile over the substantial change which had taken place\n\
in his worldly circumstances, and then wended toward headquarters to\n\
report.\n\
";

#include <stdlib.h>
#include <stdio.h>
static void
print512 (unsigned char digest[64]) {
  int i;
  
  for (i = 0; i < 64; i++)
    printf ("%x", digest[i]);
  printf ("\n");
}

#ifdef SPEED1
int main () {
  int i; uint64_t k = rand (); unsigned char out[64];
  
  for (i = 0; i < 2000000; i++)
    test (msg, 10, &out);
  printf ("10 byte: %s:", (size_t)&k & 0x7 ? "unaligned" : "aligned");
  print512 (out);
  return 0;
}
#endif

#ifdef SPEED2
int main () {
  int i; uint64_t k = rand (); unsigned char out[64];
  
  for (i = 0; i < 2000000; i++)
    test (msg, 100, &out);
  printf ("100 byte: %s:", (size_t)&k & 0x7 ? "unaligned" : "aligned");
  print512 (out);
  return 0;
}
#endif

#ifdef SPEED3
int main () {
  int i; uint64_t k = rand (); unsigned char out[64];
  
  for (i = 0; i < 2000000; i++)
    test (msg, 1000, &out);
  printf ("1000 byte: %s:", (size_t)&k & 0x7 ? "unaligned" : "aligned");
  print512 (out);
  return 0;
}
#endif

#ifdef SPEED4
int main () {
int i; uint64_t k = rand (); unsigned char out[64];

  for (i = 0; i < 500000; i++)
    test (msg, 10000, &out);
  printf ("10000 byte: %s:", (size_t)&k & 0x7 ? "unaligned" : "aligned");
  print512 (out);
  return 0;
}
#endif

