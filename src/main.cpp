#include <M5Unified.h>

// A representative set of Oblique Strategies prompts (Brian Eno / Peter Schmidt).
const char *strategies[] = {
    "Honor thy error as a hidden intention",
    "Use an old idea",
    "State the problem in words as clearly as possible",
    "Only one element of each kind",
    "What would your closest friend do?",
    "Work at a different speed",
    "Don't be afraid of things because they're easy to do",
    "Abandon normal instruments",
    "Try faking it",
    "Discover the recipes you are using and abandon them",
    "Emphasize the flaws",
    "Remove specifics and convert to ambiguities",
    "Simple subtraction",
    "Change instrument roles",
    "Look closely at the most embarrassing details and amplify them",
    "Use fewer notes",
    "Turn it upside down",
    "What mistakes did you make last time?",
    "Repetition is a form of change",
    "Ask your body",
    "Don't break the silence",
    "Consult other sources, promising, unpromising",
    "Go slowly all the way round the outside",
    "Make it more sensual",
    "Accept advice",
    "Be dirty",
    "Convert a melodic element into a rhythmic element",
    "Infinitesimal gradations",
    "Water",
    "Think of the radio",
    "Reverse",
    "Tidy up",
    "Just carry on",
    "Trust in the you of now",
    "Try to make it sentimental",
    "Breathe more deeply",
    "Allow an easement (an easement is the abandonment of a stricture)",
    "Spectrum analysis",
    "Do the washing up",
    "Disconnect from desire",
    "Give the game away",
    "Isolate the cause of the problem... and eliminate it",
    "Imagine the music as a moving chain or caterpillar",
    "Take away the elements in order of apparent non-importance",
    "You don't have to be ashamed of using your own ideas",
    "Make a blank valuable by putting it in an exquisite frame",
};
const int numStrategies = sizeof(strategies) / sizeof(strategies[0]);

unsigned long lastDrawTime = 0;
const unsigned long cooldownMs = 800;

enum ScreenState { HOME, CARD };
ScreenState screenState = HOME;
unsigned long cardShownTime = 0;
const unsigned long cardTimeoutMs = 30000;

bool screenAsleep = false;
unsigned long lastActivityTime = 0;
const unsigned long sleepTimeoutMs = 60000;

void drawCard(const char *text) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextWrap(false);

    const int marginX = 6;
    const int marginY = 10;
    const int maxWidth = M5.Display.width() - (marginX * 2);
    const int lineHeight = M5.Display.fontHeight();

    String word;
    String line;
    int cursorY = marginY;
    const char *p = text;

    auto flushLine = [&]() {
        if (line.length() > 0) {
            M5.Display.setCursor(marginX, cursorY);
            M5.Display.print(line);
            cursorY += lineHeight;
            line = "";
        }
    };

    auto handleWord = [&](const String &w) {
        if (w.length() == 0) return;
        String candidate = line.length() == 0 ? w : line + " " + w;
        if (M5.Display.textWidth(candidate) > maxWidth && line.length() > 0) {
            flushLine();
            line = w;
        } else {
            line = candidate;
        }
    };

    while (*p) {
        if (*p == ' ') {
            handleWord(word);
            word = "";
        } else {
            word += *p;
        }
        p++;
    }
    handleWord(word);
    flushLine();
}

void drawCover() {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(10, 40);
    M5.Display.println("Oblique");
    M5.Display.setCursor(10, 65);
    M5.Display.println("Strategies");
    M5.Display.setTextSize(1);
    M5.Display.setCursor(10, 110);
    M5.Display.println("shake or press A");
}

void drawRandomCard() {
    int idx = random(numStrategies);
    drawCard(strategies[idx]);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    randomSeed(esp_random());

    M5.Display.setRotation(3);
    drawCover();

    lastActivityTime = millis();
}

void wakeScreen() {
    M5.Display.wakeup();
    M5.Display.setBrightness(100);
    screenAsleep = false;
    screenState = HOME;
    drawCover();
}

void goHome() {
    screenState = HOME;
    drawCover();
}

void loop() {
    M5.update();

    bool btnPressed = M5.BtnA.wasPressed() || M5.BtnB.wasPressed();
    bool pwrPressed = M5.BtnPWR.wasClicked();

    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);
    float magnitude = sqrtf(ax * ax + ay * ay + az * az);
    // 1g at rest; a sharp shake spikes well above that.
    bool shook = magnitude > 2.2f;

    bool anyInput = btnPressed || pwrPressed || shook;
    unsigned long now = millis();

    if (screenAsleep) {
        if (anyInput) {
            wakeScreen();
            lastActivityTime = now;
        }
        delay(20);
        return;
    }

    if (anyInput) {
        lastActivityTime = now;
    }

    if (pwrPressed) {
        goHome();
    } else if ((btnPressed || shook) && (now - lastDrawTime) > cooldownMs) {
        drawRandomCard();
        lastDrawTime = now;
        screenState = CARD;
        cardShownTime = now;
    }

    if (screenState == CARD && (now - cardShownTime) > cardTimeoutMs) {
        goHome();
    }

    if ((now - lastActivityTime) > sleepTimeoutMs) {
        M5.Display.sleep();
        screenAsleep = true;
    }

    delay(20);
}
