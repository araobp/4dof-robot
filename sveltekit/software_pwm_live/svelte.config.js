import adapter from '@sveltejs/adapter-static';

/** @type {import('@sveltejs/kit').Config} */
const config = {
	kit: {
		adapter: adapter({
			pages: '../../docs/software_pwm_live',
			assets: '../../docs/software_pwm_live',
			fallback: 'index.html'
		}),
		paths: {
			base: '/beginning-physical-ai/software_pwm_live',
		},
	}
};

export default config;
