const express = require("express")
const { createClient } = require("redis")
const { v4: uuidv4 } = require("uuid")

const redisClient = createClient({
	url: "redis://redis:6379",
})

const app = express()
app.use(express.json())

app.post("/create", async (req, res) => {
	try {
		const { text } = req.body

		const newTask = {
			id: uuidv4(),
			text: text,
		}

		await redisClient.lPush("task", JSON.stringify(newTask))

		res.status(202).json({ status: "created", task: newTask })
	} catch (err) {
		console.error(err)
		res.status(500).json({ error: "ошибка сервера" })
	}
})

async function start() {
	await redisClient.connect()
	app.listen(5000, "0.0.0.0", () => {
		console.log("Сервер запущен на порту 5000")
	})
}
start()
