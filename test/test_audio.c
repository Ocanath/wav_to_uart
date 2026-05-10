#include "audio.h"
#include "unity.h"
#include <math.h>
#include <stdbool.h>

void test_audio_stream_nulptr(void)       
{
        int32_t v = audio_stream(NULL, 0);
        TEST_ASSERT_EQUAL(0, v);
}

void test_stop(void)
{
        audio_renderer_t a = {};
        int32_t v = audio_stream(&a, 13411);
        TEST_ASSERT_EQUAL(0, v);
}

void test_audio_stream_overrun(void)
{
        audio_renderer_t a = {};
        audio_stream(&a, 0);    //loads up prev_us
        a.retransmission_us = 25;       //40khz, standard
		const size_t bufsize = sizeof(a.recv_buffer)/sizeof(int16_t);
        for(int i = 0; i < bufsize; i++)
        {
                a.recv_buffer[i] = i+1;
                TEST_ASSERT_NOT_EQUAL(0, a.recv_buffer[i]);     //must be nonzero or test is confused
        }

        //overrun test - confirm nonzero return for overrun pointer
        a.buffer_pos = sizeof(a.recv_buffer) + 10;
        int32_t v = audio_stream(&a, 0);
        TEST_ASSERT_EQUAL(0, v);

        a.buffer_pos = 0;
        for(int us = 0; us < a.retransmission_us * (bufsize + 10); us += 5)
        {
                v = audio_stream(&a, us);
                if(a.buffer_pos >= bufsize)
                {
                        TEST_ASSERT_EQUAL(0, v);
                }
        }
        TEST_ASSERT_EQUAL(10 -1, a.buffer_pos);

}

void test_audio_sampling(void)
{
        audio_renderer_t a = {};
        a.retransmission_us = 25;       //40khz, standard
        for(int i = 0; i < sizeof(a.recv_buffer)/sizeof(int16_t); i++)
        {
                a.recv_buffer[i] = i+1;
                TEST_ASSERT_NOT_EQUAL(0, a.recv_buffer[i]);     //must be nonzero or test is confused
        }
        audio_stream(&a, 0);

        //sampling - confirm that a new value occurs DIRECTLY on the microsecond edge transition
        a.buffer_pos = 0;
        int32_t v = audio_stream(&a, 10);
        TEST_ASSERT_EQUAL(1, v);
        v = audio_stream(&a, 1000);
        TEST_ASSERT_EQUAL(2, v);
        v = audio_stream(&a, 1023);
        TEST_ASSERT_EQUAL(2, v);
        v = audio_stream(&a, 1024);
        TEST_ASSERT_EQUAL(2, v);
        v = audio_stream(&a, 1025);
        TEST_ASSERT_EQUAL(3, v);


        //restart buffer
        a.buffer_pos = 0;
        v = audio_stream(&a, 1035);
        TEST_ASSERT_EQUAL(1, v);
        v = audio_stream(&a, 1075);
        TEST_ASSERT_EQUAL(2, v);


}

void test_audio_overflow(void)
{
        audio_renderer_t a = {};
        a.retransmission_us = 25;       //40khz, standard
        for(int i = 0; i < sizeof(a.recv_buffer)/sizeof(int16_t); i++)
        {
                a.recv_buffer[i] = i+1;
                TEST_ASSERT_NOT_EQUAL(0, a.recv_buffer[i]);     //must be nonzero or test is confused
        }

        //integer overflow test
        int32_t v1 = audio_stream(&a, 0xFFFFFFFF-10);
        int32_t v2 = audio_stream(&a, 0xFFFFFFFF-10);
        TEST_ASSERT_EQUAL(2, v1);
        TEST_ASSERT_EQUAL(2, v2);
        v2 = audio_stream(&a, 10);
        TEST_ASSERT_EQUAL(2, v2);
        v2 = audio_stream(&a, 14);
        TEST_ASSERT_EQUAL(3, v2);
        v2 = audio_stream(&a, 16);
        TEST_ASSERT_EQUAL(3, v2);
}



void test_audio_nostream(void)
{
        audio_renderer_t a = {};
        a.retransmission_us = 25;
        for(int i = 0; i < sizeof(a.recv_buffer)/sizeof(int16_t); i++)
        {
                a.recv_buffer[i] = i+1;
                TEST_ASSERT_NOT_EQUAL(0, a.recv_buffer[i]);     //must be nonzero or test is confused
        }

        TEST_ASSERT_EQUAL(0, a.buffer_pos);
        int32_t val = 0;
        val = audio_stream_nosync(&a, 0, 50);
        TEST_ASSERT_EQUAL(1, val);
        TEST_ASSERT_EQUAL(1, a.buffer_pos);
        val = audio_stream_nosync(&a, 0, 51);
        TEST_ASSERT_EQUAL(2, val);
        TEST_ASSERT_EQUAL(1, a.buffer_pos);

        size_t firstblock = sizeof(a.recv_buffer)/sizeof(int16_t)/2;

        uint32_t tus = 52;
        for(int i = 0; i < 1000; i++)
        {
                val = audio_stream_nosync(&a, 0, tus);
                tus+=50;

                if(i == 0)
                {
                        TEST_ASSERT_EQUAL(2, val);
                        TEST_ASSERT_EQUAL(1, a.buffer_pos);
                }
                else if(i <= firstblock-1)  //take off one since we started with one
                {
                        TEST_ASSERT_EQUAL(i+1, val);    //we did one, we skip the first since the time is stale, so we start increment at 3
                }
        }
        TEST_ASSERT_EQUAL(sizeof(a.recv_buffer)/sizeof(int16_t)/2, a.buffer_pos);
        TEST_ASSERT_EQUAL(0, val);
        
        val = audio_stream_nosync(&a, 1, tus);
        TEST_ASSERT_EQUAL(firstblock+1, val);     
        TEST_ASSERT_EQUAL(firstblock+1, a.buffer_pos);    

        for(int i = 0; i < 1000; i++)
        {
                val = audio_stream_nosync(&a, 1, tus);
                tus+=50;
                if(i == 0)
                {
                        TEST_ASSERT_EQUAL(firstblock+2, val);
                }
                if(i > 0 && i <= firstblock-1)
                {
                        TEST_ASSERT_EQUAL(a.buffer_pos, val);
                }
        }
        TEST_ASSERT_EQUAL(sizeof(a.recv_buffer)/sizeof(int16_t), a.buffer_pos);
        TEST_ASSERT_EQUAL(0, val);
}

void test_audio_sample(void)
{
	audio_renderer_t a = {};
	const size_t halfsize = sizeof(a.recv_buffer)/sizeof(int16_t)/2;
	a.retransmission_us = 31;
	int16_t in = 1;
	uint32_t t_us = 32;
	uint8_t trigger = 0;
	for(int i = 0; i < 1000; i++)
	{
		trigger = audio_sample(&a, in++, t_us);
		t_us += 32;
		if(i >= halfsize-1)
		{
			TEST_ASSERT_EQUAL(1, trigger);
		}
		else
		{
			TEST_ASSERT_EQUAL(0, trigger);
		}
	}
	for(int i = 0; i < halfsize-1; i++)
	{
		TEST_ASSERT_EQUAL(i+1, a.recv_buffer[i]);
	}
	TEST_ASSERT_EQUAL(1000, a.recv_buffer[halfsize-1]);
	a.buffer_pos = halfsize;
	for(int i = 0; i < 1000; i++)
	{
		trigger = audio_sample(&a, in++, t_us);
		t_us += 32;
		if(i >= halfsize-1)
		{
			TEST_ASSERT_EQUAL(1, trigger);
		}
		else
		{
			TEST_ASSERT_EQUAL(0, trigger);
		}
	}
	for(int i = 0; i < halfsize-1; i++)
	{
		TEST_ASSERT_EQUAL(i+1001, a.recv_buffer[halfsize + i]);
	}
	TEST_ASSERT_EQUAL(2000, a.recv_buffer[halfsize + halfsize-1]);

}

