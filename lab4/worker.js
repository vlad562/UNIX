const { createClient } = require("redis")

const workerId = process.env.HOSTNAME
const redisClient = createClient({ url: "redis://redis:6379" })

const sleep = ms => new Promise(res => setTimeout(res, ms))

let run = true

process.on("SIGTERM", () => {
	run = false
})
process.on("SIGINT", () => {
	run = false
})

async function worker() {
	await redisClient.connect()
	console.log(`Worker ${workerId} запущен`)

	while (run) {
		try {
			const task = await redisClient.brPopLPush("task", "process", 5)
			if (task) {
				const { id, text } = JSON.parse(task)
				console.log(`${workerId} Взял задачу ${id}: ${text}`)

				await sleep(1000)

				await redisClient.lRem("process", 0, task)
				console.log(`${workerId} Завершил задачу ${id}`)
			}
		} catch (err) {
			console.error("Ошибка воркера:", err.message)
			await sleep(1000)
		}
	}

	await redisClient.quit()
	console.log(`Worker ${workerId} остановлен`)
}

worker()
