from locust import HttpUser, task

class WebUser(HttpUser):
    @task
    def load_index(self):
        self.client.get("/")
    @task
    def load_page2(self):
        self.client.get("/page2.html")

