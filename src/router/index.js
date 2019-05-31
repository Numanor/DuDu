import Vue from 'vue'
import Router from 'vue-router'
import CabMap from '@/components/CabMap'
import HelloWorld from '@/components/HelloWorld'

Vue.use(Router)

export default new Router({
  routes: [
    {
      path: '/',
      name: 'CabMap',
      component: CabMap
    }
  ]
})
