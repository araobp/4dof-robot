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
			base: process.env.NODE_ENV === "production" ? "/beginning_physical_ai" : "",
		},
	}
};

export default config;
