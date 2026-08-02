#include <gtest/gtest.h>
#include "soundscripts.h"

const char soundScripts[] = R"(
{
	"Civilian.Pain": {
		"waves": ["colette/colette_pain0.wav", "colette/colette_pain1.wav", "colette/colette_pain2.wav", "colette/colette_pain3.wav"],
		"channel": "voice",
		"pitch": 105,
		"volume": 0.9,
		"attenuation": "static"
	},
	"Civilian.Die": {
		"waves": ["colette/colette_die0.wav", "colette/colette_die1.wav", "colette/colette_die2.wav"],
		"channel": "body",
		"pitch": [95, 105],
		"volume": [0.9, 1.0],
		"attenuation": "norm"
	},
	"Bullsquid.Growl": {
		"waves": ["bullsquid_hl2/attackgrowl1.wav", "bullsquid_hl2/attackgrowl2.wav"],
		"channel": "weapon",
		"attenuation": 1.1
	}
}
)";

TEST(SoundScripts, Parse) {
	SoundScriptSystem s;
	ASSERT_TRUE(s.ReadFromContents(soundScripts, ""));

	{
		const SoundScript* civilianPain = s.GetSoundScript("Civilian.Pain");
		ASSERT_TRUE(civilianPain != nullptr);
		EXPECT_EQ(civilianPain->waves.size(), 4);
		EXPECT_STREQ(civilianPain->waves[0], "colette/colette_pain0.wav");
		EXPECT_STREQ(civilianPain->waves[3], "colette/colette_pain3.wav");
		EXPECT_EQ(civilianPain->channel, CHAN_VOICE);
		EXPECT_EQ(civilianPain->pitch.min, 105);
		EXPECT_EQ(civilianPain->pitch.min, civilianPain->pitch.max);
		EXPECT_EQ(civilianPain->volume.min, 0.9f);
		EXPECT_EQ(civilianPain->volume.min, civilianPain->volume.max);
		EXPECT_EQ(civilianPain->attenuation, ATTN_STATIC);
	}

	{
		const SoundScript* civilianDie = s.GetSoundScript("Civilian.Die");
		ASSERT_TRUE(civilianDie != nullptr);
		EXPECT_EQ(civilianDie->waves.size(), 3);
		EXPECT_STREQ(civilianDie->waves[0], "colette/colette_die0.wav");
		EXPECT_STREQ(civilianDie->waves[2], "colette/colette_die2.wav");
		EXPECT_EQ(civilianDie->channel, CHAN_BODY);
		EXPECT_EQ(civilianDie->pitch.min, 95);
		EXPECT_EQ(civilianDie->pitch.max, 105);
		EXPECT_EQ(civilianDie->volume.min, 0.9f);
		EXPECT_EQ(civilianDie->volume.max, 1.0f);
		EXPECT_EQ(civilianDie->attenuation, ATTN_NORM);
	}

	{
		const SoundScript* bullsquidGrowl = s.GetSoundScript("Bullsquid.Growl");
		ASSERT_TRUE(bullsquidGrowl != nullptr);
		EXPECT_EQ(bullsquidGrowl->waves.size(), 2);
		EXPECT_STREQ(bullsquidGrowl->waves[0], "bullsquid_hl2/attackgrowl1.wav");
		EXPECT_STREQ(bullsquidGrowl->waves[1], "bullsquid_hl2/attackgrowl2.wav");
		EXPECT_EQ(bullsquidGrowl->channel, CHAN_WEAPON);
		EXPECT_EQ(bullsquidGrowl->attenuation, 1.1f);
	}
}
