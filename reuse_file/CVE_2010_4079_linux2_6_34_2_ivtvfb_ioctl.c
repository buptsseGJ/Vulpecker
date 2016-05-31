
static int CVE_2010_4079_linux2_6_34_2_ivtvfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	DEFINE_WAIT(wait);
	struct ivtv *itv = (struct ivtv *)info->par;
	int rc = 0;

	switch (cmd) {
		case FBIOGET_VBLANK: {
			struct fb_vblank vblank;
			u32 trace;

			vblank.flags = FB_VBLANK_HAVE_COUNT |FB_VBLANK_HAVE_VCOUNT |
					FB_VBLANK_HAVE_VSYNC;
			trace = read_reg(0x028c0) >> 16;
			if (itv->is_50hz && trace > 312)
				trace -= 312;
			else if (itv->is_60hz && trace > 262)
				trace -= 262;
			if (trace == 1)
				vblank.flags |= FB_VBLANK_VSYNCING;
			vblank.count = itv->last_vsync_field;
			vblank.vcount = trace;
			vblank.hcount = 0;
			if (copy_to_user((void __user *)arg, &vblank, sizeof(vblank)))
				return -EFAULT;
			return 0;
		}

		case FBIO_WAITFORVSYNC:
			prepare_to_wait(&itv->vsync_waitq, &wait, TASK_INTERRUPTIBLE);
			if (!schedule_timeout(msecs_to_jiffies(50)))
				rc = -ETIMEDOUT;
			finish_wait(&itv->vsync_waitq, &wait);
			return rc;

		case IVTVFB_IOC_DMA_FRAME: {
			struct ivtvfb_dma_frame args;

			IVTVFB_DEBUG_INFO("IVTVFB_IOC_DMA_FRAME\n");
			if (copy_from_user(&args, (void __user *)arg, sizeof(args)))
				return -EFAULT;

			return ivtvfb_prep_frame(itv, cmd, args.source, args.dest_offset, args.count);
		}

		default:
			IVTVFB_DEBUG_INFO("Unknown ioctl %08x\n", cmd);
			return -EINVAL;
	}
	return 0;
}